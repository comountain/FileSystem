// the extent server implementation

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <map>
#include "extent_server.h"
#include "persister.h"

extent_server::extent_server() 
{
  im = new inode_manager();
  _persister = new chfs_persister("log"); // DO NOT change the dir name here
  // Your code here for Lab2A: recover data on startup
  std::string logs;
  //printf("1121212");
  _persister->restore_checkpoint(im);
  _persister->restore_logdata();
  //printf("2323232");
  /*this->o.open("set.txt",std::ios::out);
  this->o << "hello";*/
  restore();
  //_persister->log_entries.clear();
}

int extent_server::create(uint32_t type, extent_protocol::extentid_t &id)
{
  // alloc a new inode and return inum
  //printf("extent_server: create inode\n");
  chfs_command com(chfs_command::CMD_CREATE, txid, 0, type, std::to_string(type));
  log(com);
  id = im->alloc_inode(type);
  return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  /*std::ofstream o("set.txt",std::ios::out);
  o << "put"<<std::endl;;
  o.close();*/
  id &= 0x7fffffff;
  
  const char * cbuf = buf.c_str();
  int size = buf.size();
  std::string content = std::to_string(id) + ' ' + std::to_string(size) + ' ' + buf;
  chfs_command command(chfs_command::CMD_PUT, txid, id,0, buf);
  log(command);
  im->write_file(id, cbuf, size);
  
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  /*o << "get: "<< id << std::endl;;
  o.close();
  printf("extent_server: get %lld\n", id);*/

  id &= 0x7fffffff;

  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    free(cbuf);
  }

  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  
  /*printf("extent_server: getattr %lld\n", id);
  o << "get attr: " << id << std::endl;*/
  id &= 0x7fffffff;
  
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->get_attr(id, attr);
  a = attr;

  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  //printf("extent_server: write %lld\n", id);

  id &= 0x7fffffff;
  chfs_command com(chfs_command::CMD_REMOVE, txid, id,0, std::to_string(id));
  log(com);
  im->remove_file(id);
  return extent_protocol::OK;
}

int extent_server::log(chfs_command command)
{
  if(command.type == chfs_command::CMD_BEGIN)
  {
    ++txid;
  }
  command.id = txid;
  _persister->append_log(command);
  return 1;
}

int extent_server::restore()
{
  /*std::vector<chfs_command> logs = _persister->log_entries;
  int len = _persister->log_entries.size();
  //this->o << "put restre "<<len << std::endl;*/
  std::vector<chfs_command> logs = _persister->log_entries;
  std::vector<int> ready2can_do;
  std::vector<int> can_do;
  bool can = false;
  for(int i = 0;i < logs.size();++i){
      if(logs[i].type == chfs_command::CMD_BEGIN)
      {
        if(!can)
        {
          can = true;
          ready2can_do.clear();
        }
        else
        {
           i --;
           can = false;
           ready2can_do.clear();
        }
      }
      else if(logs[i].type == chfs_command::CMD_COMMIT)
      {
        if(!can)
        {
          printf("err: commit without begin\n");
          continue;
        }
        else
        {
          /*for(int j = 0; j < ready2can_do.size(); j++)
          {
            //can_do.push_back(ready2can_do[j]);
          }*/
          int len = ready2can_do.size();
          for (int j = 0; j < len; j++){
            chfs_command cmd = logs[ready2can_do[j]];
            txid = cmd.id;
            if(cmd.type == chfs_command::CMD_CREATE)
            {
               int type = std::stoi(cmd.content);
               im->alloc_inode(type);
            }
            else if(cmd.type == chfs_command::CMD_PUT)
            {
                int size = cmd.content.size();
                const char *c = cmd.content.c_str();
                im->write_file(cmd.inode_id,c, size);
            }
            else if(cmd.type == chfs_command::CMD_REMOVE)
            {
                im->remove_file(cmd.inode_id);
            }
          }
          ready2can_do.clear();
          can = false;
        }
      }
      else
      {
        if(can)
          ready2can_do.push_back(i);
      }
  }
  return 1;
}

int extent_server::redo()
{
  return 1;
}

int extent_server::begin()
{
  chfs_command command(chfs_command::CMD_BEGIN,txid,0,0,"");
  log(command);
  return 1;
}

int extent_server::commit()
{
  chfs_command command(chfs_command::CMD_COMMIT,txid,0,0,"");
  log(command);
  return 1;
}

