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
    int inode_id = 3;
    std::string content;

    // constructor
    chfs_command() {}

    chfs_command(cmd_type tp, txid_t i, std::string con){
        type = tp;
        id = i;
        content = con;
    }

    uint64_t size() const {
        uint64_t s = sizeof(cmd_type) + sizeof(txid_t);
        return s;
    }

    
    void serialize(char* buf) {
        std::cout << "start ser!" << std::endl;
        memcpy(buf, &type, sizeof(cmd_type));
        memcpy(buf + sizeof(cmd_type), &id, sizeof(txid_t));
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t), &inode_id, sizeof(uint64_t));
        int content_size = content.size();
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t), &content_size, 4);
        memcpy(buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t) + 4, content.c_str(), content_size);
        std::cout << "no err" << std::endl;
    }

    void deserialize(const char* buf){
        memcpy(&type, buf, sizeof(cmd_type));
        memcpy(&id, buf + sizeof(cmd_type), sizeof(txid_t));
        memcpy(&inode_id, buf + sizeof(cmd_type) + sizeof(txid_t), sizeof(uint64_t));
        int content_size;
        memcpy(&content_size, buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t), 4);
        content.resize(content_size);
        memcpy(&content[0], buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(uint64_t) + 4, content_size);
    }
     
    uint32_t getSize(){
        uint32_t s = 4 + sizeof(cmd_type) + sizeof(txid_t) + content.length();
        return s;
    }

    std::string write_buf() const
    {
        /*char *tmp = buf;
        int len = content.size();
        //std::cout << "this log is: " << tmp << std::endl;
        *(cmd_type *)tmp = type;
        std::cout << "this log type is: " << *(cmd_type *)tmp << std::endl;
        *(txid_t *)(tmp + sizeof(cmd_type)) = id;
        std::cout << "this log id is: " << *(txid_t *)(tmp + sizeof(cmd_type)) << std::endl;
        *(int *)(tmp + sizeof(cmd_type) + sizeof(txid_t)) = len;
        //memcpy(tmp + sizeof(cmd_type) + sizeof(txid_t) + sizeof(int), content.c_str(), len);
        char *t = tmp + sizeof(cmd_type) + sizeof(txid_t) + sizeof(int);
        for(int i = 0; i < len; i++)
        {
            t[i] = content[i];
        }
        std::cout << t;
        std::cout << content << std::endl;
        std::cout << "this log is: " << buf + sizeof(cmd_type) + sizeof(txid_t) + sizeof(int)<< std::endl;*/
        int len = content.size();
        std::string to_w = ' ' + std::to_string(type)+ ' ' + std::to_string(id) + ' ' + std::to_string(len) +' '+ content;
        //std::cout << to_w << std::endl << to_w.size() << std::endl;
        return to_w;
    }
};

std::vector<chfs_command> log_entries;
std::string file_path_logfile = "log/logdata.bin";
std::string file_path_sizefile = "log/data.bin";
int log_fd = open(file_path_logfile.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);
std::ofstream on;
void append_log(chfs_command &log) {
    /*log_entries.push_back(log);
    int num = log_entries.size();
    int log_num_fd;
    log_num_fd = open(file_path_sizefile.c_str(), O_CREAT|O_RDWR, S_IRUSR | S_IWUSR);

   /* std::ifstream in;
    in.open(file_path_logfile, std::ios::binary);
    int log_size;
    in.read((char *)&log_size, 4);
   // std::cout << "log size: " << log_size << std::endl;*/
    /*write(log_num_fd, &num, sizeof(int));

    //int len = sizeof(chfs_command::cmd_type) + sizeof(chfs_command::txid_t) + log.content.size() + sizeof(int);
    //std::cout << "len: " << len << std::endl;
    std::string buf = log.write_buf();
    //write(log_fd, &len, sizeof(int));
    on << buf;
    //write(log_fd, buf.c_str(), buf.size());
    std::cout << buf << std::endl;*/
   // std::cout << "log is: " << buf + sizeof(chfs_command::cmd_type) + sizeof(chfs_command::txid_t) + sizeof(int)<< std::endl;
    std::cout << "start" << std::endl;
    char* buf;
    uint32_t size = log.size();
    std::cout << "size is: " << size << std::endl;
    write(log_fd, &size, sizeof(uint32_t));
    std::cout << "start get char" << std::endl;
    buf = new char [size];
    std::cout << "start to call" << std:: endl;
    log.serialize(buf);
    std::cout << "finish ser:" <<buf << std::endl;
    write(log_fd, buf, size);
    std::cout << "finish wr" << std::endl;
    //delete buf;
}


int main()
{
   /* int num = 8;
    write(log_fd, &num, 4);*/
    
    
   
    /*std::ifstream in(file_path_logfile, std::ios::binary);
    int log_size, consize;
    char p;
    chfs_command::cmd_type typ;
    chfs_command::txid_t id;
    std::string con;*/
    /*in.read((char *)&log_size, 4);
    std::cout << "now size: " << log_size << std::endl;*/
    /*for(int i = 0; i < log_size; i++)
    {
        in.read((char *)&p, 1);
        std::cout << "it's " << i << std::endl;
        in.read((char *)&typ, sizeof(chfs_command::cmd_type));
        in.read((char *)&p, 1);
        std::cout << "the type is :" << typ << std::endl;
        in.read((char *)&id, sizeof(chfs_command::txid_t));
        in.read((char *)&p, 1);
        std::cout << "the txid is :" << id << std::endl;
        in.read((char *)&consize, 4);
        char *buf = new char[consize];
        in.read(buf, consize);
        std::cout << "the content size is: " << consize << std::endl;
        con = buf;
        chfs_command com(typ,id,con);
        log_entries.push_back(com);
        std::cout << typ << " " << id << " " << con << std::endl;

    }*/
   /* std::ifstream inn(file_path_sizefile,std::ios::binary);
    int len;
    inn.read((char *)&len, 4);
    std::cout << "size is: " << len << std::endl;
    std::stringstream ins;
    ins << in.rdbuf();
   // printf("%s" , ins.str().c_str()) ;
    
    for(int i = 0; i < len; i++)
    {
        int typ, si;
        unsigned long long  id;
        ins.get();
        ins >> typ;
        ins.get();
        ins >> id;
        ins.get();
        ins >> si;
        //std::cout << id << std::endl;
        char bud[si];
        ins.get();
        for(int i = 0; i < si; i++)
            ins.get(bud[i]);
        std::cout << bud << std::endl;
     
    }*/
   int n = 100;
   float re = 1;
   long m = 4;
   for(int i = 3; i <=n; i++)
   {
       m *= 2;
       if(i %2 == 0)
       continue;
       re *= 4*(double) (i-2)/(i+1);
       //std::cout << re << std::endl;
       float c = re / m;
       std::cout << c << std::endl;
       if(c *(i+1) >= 4)
       {
           std::cout << c << " " << i;
           break;
       } 
   }
   std::cout << re;
}