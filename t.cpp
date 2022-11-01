#include <sstream>
#include <stdio.h>
#include <fcntl.h>
#include <mutex>
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LOG_SZ 1024



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

    cmd_type type = CMD_BEGIN;
    txid_t id = 0;
    std::string content;
    int inode_id = 0;

    // constructor
    chfs_command() {}

    chfs_command(cmd_type tp, txid_t i, std::string con):type(tp), id(i), content(con) {}

    uint64_t size() const {
        uint64_t s = sizeof(cmd_type) + sizeof(txid_t);
        return s;
    }

    void write_buf(char *buf) 
    {
        /*char *tmp = buf;
        int len = content.size();
        //std::cout << "this log is: " << tmp << std::endl;
        *(cmd_type *)tmp = type;
        std::cout << "this log type is: " << *(cmd_type *)tmp << std::endl;
        *(txid_t *)(tmp + sizeof(cmd_type)) = id;
        std::cout << "this log id is: " << *(txid_t *)(tmp + sizeof(cmd_type)) << std::endl;
        *(int *)(tmp + sizeof(cmd_type) + sizeof(txid_t)) = len;
        memcpy(tmp + sizeof(cmd_type) + sizeof(txid_t) + sizeof(int), content.c_str(), len);
        std::cout << "this log is: " << tmp + sizeof(cmd_type) + sizeof(txid_t) + sizeof(int)<< std::endl;*/
        memcpy(buf, &type, sizeof(cmd_type));
        memcpy(buf + sizeof(cmd_type), &id, sizeof(txid_t));
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t), &inode_id, sizeof(uint64_t));
        int content_size = content.size();
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t), &content_size, 4);
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t) + 4, content.c_str(), content_size);
    }

     void deserialize(const char* buf){
        memcpy(&type, buf, sizeof(cmd_type));
        memcpy(&id, buf + sizeof(cmd_type), sizeof(txid_t));
        memcpy(&inode_id, buf + sizeof(cmd_type) + sizeof(txid_t), sizeof(uint64_t));
        int content_size;
        memcpy(&content_size, buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t), 4);
        content.resize(content_size);
        memcpy(&content[0], buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t) + 4, content_size);
    };

    uint32_t get_size() const {
        uint32_t s = 4 + sizeof(cmd_type) + sizeof(txid_t)+ sizeof(uint64_t) + content.length();
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
    void checkpoint();

    // restore data from solid binary file
    // You may modify parameters in these functions
    int restore_logdata();
    void restore_checkpoint();
   /* std::string tran_to_string(chfs_command command);
    chfs_command tran_to_com(std::string s);*/
    std::vector<command> log_entries;


private:
    bool if_exist;
    bool exists(const std::string& name);
    std::mutex mtx;
    std::string file_dir;
    std::string file_path_checkpoint;
    std::string file_path_logfile;
    int log_fd;
    std::ofstream o;
    // restored log data
   
    
};

template<typename command>
bool persister<command>::exists (const std::string& name) {
    if (FILE *file = fopen(name.c_str(), "r")) {
        fclose(file);
        return true;
    } else {
        return false;
    }   
}


template<typename command>
persister<command>::persister(const std::string& dir){
    // DO NOT change the file names here
    if(!exists("se.txt"))
    {
        if_exist = false;
        o.open("se.txt",std::ios::out);
    }
    file_dir = dir;
    file_path_checkpoint = file_dir + "/checkpoint.bin";
    file_path_logfile = file_dir + "/logdata.bin";
    log_fd = open(file_path_logfile.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
    /*int log_num_fd = open(file_path_logfile.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
    int num = 0;
    write(log_fd, &num, sizeof(int));
    write(log_num_fd, &num, sizeof(int));*/
}

template<typename command>
persister<command>::~persister() {
    // Your code here for lab2A
}
/*
template<typename command>
std::string persister<command>::tran_to_string(chfs_command command) {
    std::string ret = std::to_string(command.id) + " " + std::to_string(command.type) + " " + command.content;
    return ret;
}

template<typename command>
chfs_command persister<command>::tran_to_com(std::string s) {
    unsigned long long tid;
    int type;
    std::string con;
    s >> tid >> type >> con;
    chfs_command ret;
    ret.id = tid;
    ret.type = type;
    ret.content = con;
}*/

template<typename command>
void persister<command>::append_log(command& log) {
    // Your code here for lab2A
    /*log_entries.push_back(log);
    int num = log_entries.size();
    int log_num_fd;
    log_num_fd = open(file_path_logfile.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
    int len = sizeof(chfs_command::cmd_type) + sizeof(chfs_command::txid_t) + log.content.size() + sizeof(int);
    write(log_num_fd, &num, sizeof(int));
    char *buf = new char[len];
    write(log_fd, &len, sizeof(int));
    log.write_buf(buf);
    write(log_fd, buf, len);*/
    mtx.lock();
    if(!if_exist)
    {
        o << "append log type is: " << log.type << std::endl;
    }
    printf("append log type is: %d", log.type);
    char* buf;
    uint32_t size = log.get_size();
    write(log_fd, &size, sizeof(uint32_t));
    buf = new char [size];
    log.write_buf(buf);
    if(!if_exist)
    {
        o << "append log is: " << buf << std::endl;
    }
    write(log_fd, buf, size);
    mtx.unlock();
}

template<typename command>
void persister<command>::checkpoint() {
    // Your code here for lab2A
    close(log_fd);
}

template<typename command>
int persister<command>::restore_logdata() {
    // Your code here for lab2A
    /*printf("start");
    if(!if_exist)
    {
        printf("none");
        return 0;
    }
    std::ifstream in;
    in.open(file_path_logfile, std::ios::binary);
    int log_size, consize, len;
    chfs_command::cmd_type typ;
    chfs_command::txid_t ids;
    std::string con;
    int tx = 0;
    in.read((char *)&log_size, 4);
    printf("the log size from is: %d", log_size);
    for(int i = 0; i < log_size; i++)
    {
        in.read((char *)&len,4);
        in.read((char *)&typ, sizeof(chfs_command::cmd_type));
        in.read((char *)&ids, sizeof(chfs_command::txid_t));
        in.read((char *)&consize, 4);
        printf("len is: %d, type is: %d, id is: %d, consize is: %d",len,typ,ids,consize);
        char *buf = new char[consize];
        in.read(buf, consize);
        con = buf;
        chfs_command com(typ,ids,con);
        tx = ids;
        log_entries.push_back(com);
    }
    return tx;*/
    mtx.lock();
    printf("start restore");
    int n;
    int ret;
    uint32_t size;
    while((n = read(log_fd, &size, sizeof(uint32_t))) == sizeof(uint32_t)){
       if(!if_exist)
        o << "aha"<<std::endl;;
        char* buf = new char [size];
        if(read(log_fd, buf, size)!= size)
            printf("Read error\n");
        else{
            command cmd;
            cmd.deserialize(buf);
            printf("type:%d, id:%d",cmd.type,cmd.id);
            printf("con: %s",cmd.content);
            ret = cmd.id;
            log_entries.push_back(cmd);
        }
    }
    printf("end restore");
    mtx.unlock();
    return ret;
};

template<typename command>
void persister<command>::restore_checkpoint() {
    // Your code here for lab2A

};

using chfs_persister = persister<chfs_command>;


int main()
{
   chfs_persister _per("log");
   _per.restore_logdata();
   /*for(int i = 0; i < 100; i++)
   {
       std::cout << i << std::endl;;
       chfs_command com(chfs_command::CMD_CREATE,1,"2");
       _per.append_log(com);
   }*/
   return 0;
}