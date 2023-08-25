// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASHSIZE 13

struct {

  struct buf head[HASHSIZE];
  struct buf hash[HASHSIZE][NBUF];
  struct spinlock hashlock[HASHSIZE];
  
  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.

} bcache;

void
binit(void)
{
  struct buf *b;

  for(int i=0; i<HASHSIZE; i++){

    initlock(&bcache.hashlock[i], "bcache");
    bcache.head[i].prev = &bcache.head;
    bcache.head[i].next = &bcache.head;

    for(b = bcache.hash[i]; b < bcache.hash[i]+NBUF; b++){
      b->next = bcache.head[i].next;
      b->prev = &bcache.head[i];
      initsleeplock(&b->lock, "buffer");
      bcache.head[i].next->prev = b;
      bcache.head[i].next = b;
    }
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  uint seq = blockno % HASHSIZE;

  acquire(&bcache.hashlock[seq]);

  // Is the block already cached?
  for(b = bcache.head[seq].next; b != &bcache.head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.hashlock[seq]);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.head[seq].prev; b != &bcache.head[seq]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.hashlock[seq]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  
  uint seq = b->blockno % HASHSIZE;
  acquire(&bcache.hashlock[seq]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.head[seq].next;
    b->prev = &bcache.head[seq];
    bcache.head[seq].next->prev = b;
    bcache.head[seq].next = b;
  }
  
  release(&bcache.hashlock[seq]);
}

void
bpin(struct buf *b) {
    uint seq = b->blockno % HASHSIZE;
  acquire(&bcache.hashlock[seq]);
  b->refcnt++;
  release(&bcache.hashlock[seq]);
}

void
bunpin(struct buf *b) {
    uint seq = b->blockno % HASHSIZE;
  acquire(&bcache.hashlock[seq]);
  b->refcnt--;
  release(&bcache.hashlock[seq]);
}


