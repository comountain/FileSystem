#ifndef persister_h
#define persister_h

#include <fcntl.h>
#include <mutex>
#include <iostream>
#include <fstream>
#include "rpc.h"
#include <unistd.h>
#include <sys/types.h>
#include <vector>
#define MAX_LOG_SZ 131072

/*
 * Your code here for Lab2A:
 * Implement class chfs_command, you may need to add command types such as
 * 'create', 'put' here to represent different commands a transaction requires. 
 * 
 * Here are some tips:
 * 1. each transaction in ChFS consists of several chfs_commands.
 * 2. each transaction in ChFS MUST contain a BEGIN command and a COMMIT command.
 * 3. each chfs_commands contains transaction ID, command type, and other information.
 * 4. you can treat a chfs_command as a log entry.
 */
class chfs_command {
public:
    typedef unsigned long long txid_t;
    enum cmd_type {
        CMD_BEGIN = 0,
        CMD_COMMIT,
        CMD_CREATE,
        CMD_PUT,
        CMD_REMOVE

    };

    cmd_type type;
    txid_t id;
    uint32_t inode_id = 0;
    uint32_t inode_type = 0;
    std::string content;

    // constructor
    chfs_command():type(CMD_BEGIN),id(0),content(""){};

    chfs_command(cmd_type tp, txid_t id, uint32_t inode_id, uint32_t inode_type, const std::string& content): type(tp), id(id), inode_id(inode_id), inode_type(inode_type), content(content){};
    
    chfs_command(const chfs_command &cmd): type(cmd.type), id(cmd.id), inode_id(cmd.inode_id), inode_type(cmd.inode_type),content(cmd.content){} ;

    chfs_command(const char *buf)
    {
        memcpy(&type, buf, sizeof(cmd_type));
        memcpy(&id, buf + sizeof(cmd_type), sizeof(txid_t));
        memcpy(&inode_id, buf + sizeof(cmd_type) + sizeof(txid_t), sizeof(uint32_t));
        memcpy(&inode_type, buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint32_t), sizeof(uint32_t));
        int content_size;
        memcpy(&content_size, buf + sizeof(cmd_type) + sizeof(txid_t) + 2 * sizeof(uint32_t), 4);
        content.resize(content_size);
        memcpy(&content[0], buf + sizeof(cmd_type) + sizeof(txid_t) + 2 * sizeof(uint32_t) + 4, content_size);
    }

    char* turn_to_char(){
        int len =  4 + sizeof(cmd_type) + sizeof(txid_t)+ 2*sizeof(uint32_t) + content.length();
        char *buf = new char[len];
        memcpy(buf, &type, sizeof(cmd_type));
        memcpy(buf + sizeof(cmd_type), &id, sizeof(txid_t));
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t), &inode_id, sizeof(uint32_t));
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint32_t), &inode_type, sizeof(uint32_t));
        int content_size = content.size();
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t) + 2*sizeof(uint32_t), &content_size, 4);
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t) + 2*sizeof(uint32_t) + 4, content.c_str(), content_size);
        return buf;
    };

    uint64_t size() const {
        uint64_t s = 4 + sizeof(cmd_type) + sizeof(txid_t)+ 2*sizeof(uint32_t) + content.length();
        return s;
    }

};

/*
 * Your code here for Lab2A:
 * Implement class persister. A persister directly interacts with log files.
 * Remember it should not contain any transaction logic, its only job is to 
 * persist and recover data.
 * 
 * P.S. When and how to do checkpoint is up to you. Just keep your logfile size
 *      under MAX_LOG_SZ and checkpoint file size under DISK_SIZE.
 */
template<typename command>
class persister {

public:
    persister(const std::string& file_dir);
    ~persister();

    // persist data into solid binary file
    // You may modify parameters in these functions
    void append_log(command& log);
    int checkpoint();

    // restore data from solid binary file
    // You may modify parameters in these functions
    void restore_logdata();
    void restore_checkpoint(inode_manager *im);
   
    std::vector<command> log_entries;

private:
    std::mutex mtx;
    std::string file_dir;
    std::string file_path_checkpoint;
    std::string file_path_logfile;
    std::string file_path_disk;
    int check_fd;
    int log_fd;
    int log_size;
    // restored log data
    
};

template<typename command>
persister<command>::persister(const std::string& dir){
    // DO NOT change the file names here
    log_size = 0;
    file_dir = dir;
    file_path_checkpoint = file_dir + "/checkpoint.bin";
    file_path_logfile = file_dir + "/logdata.bin";
    //log_fd = open(file_path_checkpoint.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
    check_fd = open(file_path_checkpoint.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
    log_fd = open(file_path_logfile.c_str(),  O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
}

template<typename command>
persister<command>::~persister() {
    // Your code here for lab2A
    close(log_fd);
    close(check_fd);
}

template<typename command>
void persister<command>::append_log(command& log) {
    // Your code here for lab2A
    log_entries.push_back(log);
    uint64_t size = log.size();
    char *buf = log.turn_to_char();
    //printf("append log of size: %d\n", size);
    write(log_fd, &size, sizeof(uint64_t));
    write(log_fd, buf, size);
    log_size += size;
    if(log_size >= MAX_LOG_SZ)
        checkpoint();
    
}

template<typename command>
int persister<command>::checkpoint() {
    // Your code here for lab2A
    log_size = 0;
    int ret = 0;
    //printf("star:map<chfs_cot to checkpoint\n");
    std::map<chfs_command::txid_t,int> commited;
    std::vector<command> restored;
    int len = log_entries.size();
    for(int i = 0; i < len; i++)
    {
        //printf("from logfile, type is %d, id is %d\n", log_entries[i].type, log_entries[i].id);
        if(log_entries[i].type == chfs_command::CMD_COMMIT)
        {
            //printf("been commited\n");
            ret = i;
            commited.insert(std::pair<chfs_command::txid_t,int>(log_entries[i].id,1));
        }
    } 
    close(log_fd);
    std::fstream fout(file_path_logfile, std::ios::out | std::ios::trunc);
    fout.write("",0);
    fout.close();
    //exit(0);
    log_fd = open(file_path_logfile.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
    for(int i = ret + 1; i < len; i++)
    {
        restored.push_back(log_entries[i]);
        uint64_t size = log_entries[i].size();
        char *buf = log_entries[i].turn_to_char();
        //printf("append log of size: %d\n", size);
        write(log_fd, &size, sizeof(uint64_t));
        write(log_fd, buf, size);
        log_size += size;
    }
    for(int i = 0; i <= ret; i++)
    {
        if(commited.find(log_entries[i].id) != commited.end())
        {
            //printf("write a log to checkpoint\n");
            uint64_t size = log_entries[i].size();
            char *buf = log_entries[i].turn_to_char();
            write(check_fd, &size, sizeof(uint64_t));
            write(check_fd, buf, size);
        }
    }
    log_entries.clear();
    for(int i = 0; i < restored.size(); i++)
        log_entries.push_back(restored[i]);
    return ret;
}

template<typename command>
void persister<command>::restore_logdata() {
    // Your code here for lab2A
    //printf("start to restore_log\n");
    uint64_t size;
    while(read(log_fd, &size, sizeof(uint64_t)) == sizeof(uint64_t))
    {
        //printf("find a log log\n");
        char* buf = new char [size];
        if(read(log_fd, buf, size) == size)
        {
            command cmd(buf);
            //printf("from logfile, type is %d, id is %d\n", cmd.type, cmd.id);
            log_entries.push_back(cmd);
            log_size += size;
        }
    }
};

template<typename command>
void persister<command>::restore_checkpoint(inode_manager *im) {
    // Your code here for lab2A
    //printf("start to restore_checkpoint\n");
    uint64_t size;
    std::vector<chfs_command> logs;
    while(read(check_fd, &size, sizeof(uint64_t)) == sizeof(uint64_t))
    {
        //printf("find a point log\n");
        char* buf = new char [size];
        if(read(check_fd, buf, size) == size)
        {
            command cmd(buf);
            //printf("from chekcpoint, type is %d, id is %d\n", cmd.type, cmd.id);
            logs.push_back(cmd);
        }
    }
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
            //printf("err: commit without begin\n");
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
};




using chfs_persister = persister<chfs_command>;

#endif // persister_h