#include "inode_manager.h"

// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{
  if(id < 0 || id >= BLOCK_NUM)
    return;
  memcpy(buf, blocks[id], BLOCK_SIZE);
}

void
disk::write_block(blockid_t id, const char *buf)
{
  if(id < 0 || id >= BLOCK_NUM)
    return;
  memcpy(blocks[id], buf, BLOCK_SIZE);
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
  int first = IBLOCK(sb.ninodes,sb.nblocks);
  for(int i = first + 1; i < sb.nblocks; i++)
  {
    std::map<uint32_t,int>::iterator result = using_blocks.find(i);
    if(result != using_blocks.end())
      continue;
    using_blocks.insert(std::pair<uint32_t,int>(i,1));
    return i;
  }
  return 0;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  std::map<uint32_t,int>::iterator result = using_blocks.find(id);
  if(result == using_blocks.end())
    return;
  using_blocks.erase(result);
  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */
  struct inode *ino;
  for(int i = 1; i <= INODE_NUM; i++)
  {
    //printf("\tcheck %d\n",i);
    ino = get_inode(i);
    //printf("next is check type %d\n",ino->type);
    if(ino->type == extent_protocol::T_FREE)
    {
     // printf("\tfind it %d\n",i);
      time_t rawTime;
      ino->atime = time(&rawTime);
      ino->ctime = ino->atime;
      ino->mtime = ino->atime;
      ino->size = 0;
      ino->bn = 0;
      ino->type = type;
      put_inode(i,ino);
      //ino = get_inode(i);
      //printf("type is : %d", ino->type);
      //delete ino;
      //delete ino;
      return i;
    }
  }
  //delete ino;
  return -1;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */
  struct inode *ino = get_inode(inum);
  if(ino == NULL)
    return;
  if(ino->type == extent_protocol::T_FREE)
  {
    delete ino;
    return;
  }
  ino->type = extent_protocol::T_FREE;
  put_inode(inum,ino);
  delete ino;
  return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino;
  /* 
   * your code goes here.
   */
  //printf("\tim:get_inode %d\n",inum);
  char buf[BLOCK_SIZE];
  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  if(inum < 0 || inum >= INODE_NUM || inum == NULL)
  {
    printf("\tinum err! param: %d\n",inum);
    return nullptr;
  }
  struct inode *ino_disk;
  ino_disk = (struct inode*)buf + inum%IPB;
  //printf("\tthe type of inode is: %d\n", ino_disk->type);
  ino = (struct inode*)malloc(sizeof(struct inode));
  *ino = *ino_disk;

  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out
   */
  struct inode *ino = get_inode(inum);
  if(ino == nullptr || ino->type == extent_protocol::T_FREE)
  {
    return;
  }  
  printf("num: %d\n, size: %d\n",ino->bn,ino->size);
  *size = ino->size;
  time_t rawTime;
  ino->atime = time(&rawTime);
  char *extent = new char[ino->size];
  if(ino->bn < NDIRECT)
  {
    for(int i = 0; i < ino->bn; i++)
    {
      char *temp = new char[BLOCK_SIZE];
      bm->read_block(ino->blocks[i],temp);
      int rs = BLOCK_SIZE;
      if(i == ino->bn - 1)
        rs = ino->size - i * BLOCK_SIZE;
      memcpy(extent+i*BLOCK_SIZE,temp,rs);
      delete temp;
    }
  }
  else
  {
    int i;
    for(i = 0; i < NDIRECT; i++)
    {
      char *temp = new char[BLOCK_SIZE];
      bm->read_block(ino->blocks[i],temp);
      int rs = BLOCK_SIZE;
      if(i == ino->bn - 1)
        rs = ino->size - i * BLOCK_SIZE;
      memcpy(extent+i*BLOCK_SIZE,temp,rs);
      delete temp;
    }
    int blockId = ino->blocks[NDIRECT];
    char *temp = new char[BLOCK_SIZE];
    bm->read_block(blockId,temp);
    uint32_t *lesId = (uint32_t *)temp;
    for(; i < ino->bn; i++)
    {
      char *temp = new char[BLOCK_SIZE];
      bm->read_block(lesId[i-NDIRECT],temp);
      int rs = BLOCK_SIZE;
      if(i == ino->bn - 1)
        rs = ino->size - i * BLOCK_SIZE;
      memcpy(extent+i*BLOCK_SIZE,temp,rs);
      delete temp;
    }

  }
  *buf_out = extent;
  delete ino;
  return;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
  struct inode *ino = get_inode(inum);
  if(ino == nullptr || ino->type == extent_protocol::T_FREE)
  {
    return;
  }
  int bnum = size / BLOCK_SIZE;
  if(size % BLOCK_SIZE != 0)
      bnum ++;
  printf("size num is: %d\n",bnum);
  time_t rawTime;
  int timeNow = time(&rawTime);
  ino->atime = timeNow;
  ino->ctime = timeNow;
  ino->mtime = timeNow;
  ino->size = size;
  if(ino->bn > bnum)
  {
    printf("pre larger\n");
    if(bnum > NDIRECT)
    {
      char *buf = new char[BLOCK_SIZE];
      bm->read_block(ino->blocks[NDIRECT],buf);
      uint32_t *lesIds = (uint32_t *)buf;
      for(int i = bnum; i < ino->bn; i++)
      {
        bm->free_block(lesIds[i-NDIRECT]);
      }
    }
    else if(ino->bn > NDIRECT)
    {
      char *buf = new char[BLOCK_SIZE];
      bm->read_block(ino->blocks[NDIRECT],buf);
      uint32_t *lesIds = (uint32_t *)buf;
      for(int i = NDIRECT; i < ino->bn; i++)
      {
        bm->free_block(lesIds[i-NDIRECT]);
      }
      for(int i = bnum; i < NDIRECT; i++)
      {
        bm->free_block(ino->blocks[i]);
      }
    }
    else
    {
      for(int i = bnum; i < ino->bn; i++)
      {
        int blockId = ino->blocks[i];
        bm->free_block(blockId);
      }
    }
    ino->bn = bnum;
  }
  else if(ino->bn < bnum)
  {
    printf("now larger\n");
    if(ino->bn > NDIRECT)
    {
      printf("pre larger than ND\n");
      int lesId = ino->blocks[NDIRECT];
      printf("lesId: %d\n",lesId);
      char *buf = new char[BLOCK_SIZE];
      bm->read_block(lesId,buf);
      uint32_t *lesIds = (uint32_t *)buf;
      for(int i = ino->bn; i < bnum; i++)
      {
        lesIds[i-NDIRECT] = bm->alloc_block();
      } 
      bm->write_block(lesId,buf);
    }
    else if(bnum > NDIRECT)
    {
      printf("now larger than ND\n");
      for(int i = ino->bn; i < NDIRECT; i++)
      {
        int blockId = bm->alloc_block();
        ino->blocks[i] = blockId;
      }
      int las = bm->alloc_block();
      ino->blocks[NDIRECT] = las;
      char *buf = new char[BLOCK_SIZE];
      uint32_t *lesId = (uint32_t *)buf;
      for(int i = 0; i < bnum - NDIRECT; i++)
      {
        lesId[i] = bm->alloc_block();
      }
      bm->write_block(las, buf);
    }
    else
    {
      for(int i = ino->bn; i < bnum; i++)
      {
        int blockId = bm->alloc_block();
        ino->blocks[i] = blockId;
        printf("alloc id: %d\n", blockId);
      }
    }
    ino->bn = bnum;
  }
   if(bnum <= NDIRECT)
  {
    for(int i = 0; i < bnum; i++)
    {
      printf("write %d\n", ino->blocks[i]);
      bm->write_block(ino->blocks[i],buf+ i * BLOCK_SIZE);
    }
  }
  else
  {
    for(int i = 0; i < NDIRECT; i++)
    {
      printf("write %d\n", ino->blocks[i]);
      bm->write_block(ino->blocks[i],buf+ i * BLOCK_SIZE);
    }
    char *buff = new char[BLOCK_SIZE];
    bm->read_block(ino->blocks[NDIRECT],buff);
    uint32_t *lesIds = (uint32_t *)buff;
    for(int i = NDIRECT; i < bnum; i++)
    {
      bm->write_block(lesIds[i-NDIRECT],buf+i*BLOCK_SIZE);
    }
  }
  put_inode(inum,ino);
  return;
}

void
inode_manager::get_attr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */
  struct inode *ino = get_inode(inum);
  if(ino == nullptr || ino->type == extent_protocol::T_FREE)
    return;
  a.atime = ino->atime;
  a.ctime = ino->ctime;
  a.mtime = ino->mtime;
  a.size = ino->size;
  a.type = ino->type;
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  struct inode *ino = get_inode(inum);
  if(ino == nullptr)
    return;
  if(ino->type == extent_protocol::T_FREE)
  {
    delete ino;
    return;
  }
  for(int i = 0; i < ino->bn; i++)
  {
    int blockId = ino->blocks[i];
    bm->free_block(blockId);
  }
  free_inode(inum);
  delete ino;
  return;
}
