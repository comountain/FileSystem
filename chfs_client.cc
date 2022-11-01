// chfs client.  implements FS operations using extent and lock server
#include "chfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <fstream>

std::map<std::string,chfs_client::inum> stringSplit(std::string str, const char split)
{
    
    int len = str.length();
    int pos = 0;
    std::map<std::string,chfs_client::inum> ret;
    ret.clear();
    int lastPos = 0;
    std::string name;
    for(int i = 0; i < len; i ++)
    {
        if(str[i] == split)
        {
            if(!pos)
            {
                name = str.substr(lastPos,i-lastPos);
                lastPos = i+1;
                pos = 1 - pos;
            }
            else{
                chfs_client::inum in = chfs_client::n2i(str.substr(lastPos,i-lastPos));
                ret.insert(std::pair<std::string,chfs_client::inum>(name,in));
                lastPos = i+ 1;
                pos = 1- pos;
            }
        }
    }
    return ret;
}
/*
chfs_client::chfs_client()
{
    //o.open("si.txt", std::ios::binary);
    printf("start!!!!!");
    ec = new extent_client();
    printf("finishahhh");

}*/

chfs_client::chfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client(extent_dst);
    lc = new lock_client(lock_dst);
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

chfs_client::inum chfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string chfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool chfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        return true;
    }
    //printf("isfile: %lld is not a file\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 *
 * */
bool chfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        //printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        //printf("isdir: %lld is a dir\n", inum);
        return true;
    }
    //printf("isdir: %lld is not a dir\n", inum);
    return false;
}

bool chfs_client::issymlink(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        //printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_SYM) {
        //printf("issymlink: %lld is a symlink\n", inum);
        return true;
    }
    //printf("issymlink: %lld is not a symlink\n", inum);
    return false;
}

int chfs_client::readlink(inum ino, std::string &link)
{
    if (!issymlink(ino)) {
        return IOERR;
    }
    if (ec->get(ino, link) != extent_protocol::OK) {
        //printf("error with get\n");
        return IOERR;
    }

    return OK;
}

int chfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int chfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    //printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}

int chfs_client::getsymlink(inum inum, symlinkinfo &sin)
{
    int r = OK;
    //printf("getsymlink %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    sin.atime = a.atime;
    sin.mtime = a.mtime;
    sin.ctime = a.ctime;
    sin.size = a.size;
    //printf("getsymlink %016llx -> sz %llu\n", inum, sin.size);
release:
    return r;
}

#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int chfs_client::setattr(inum ino, size_t size)
{
    /*
     * your lab2 code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */
    //o << "set: " << ino << std::endl;
    extent_protocol::attr a;
    ec->begin();
    if (ec->getattr(ino, a) != extent_protocol::OK) {
        //printf("error getting attr\n");
        return IOERR;
    }
    if (size == a.size)
    {
        ec->commit();
        return OK;
    }

    std::string buf;
    if (ec->get(ino, buf) != extent_protocol::OK) {
        //printf("error with get\n");
        return IOERR;
    }
    buf.resize(size);
    if (ec->put(ino, buf) != extent_protocol::OK) {
        //printf("error with put\n");
        return IOERR;
    }
    ec->commit();
    return OK;
}

int chfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    printf("!!!!!!!!!!!!!!\nCREATE");
    printf(name);
    printf("CREATEEEEEEE!!!!!!!!!!!!!\n");
    if (!isdir(parent)) {
        return IOERR;
    }
    bool found = false;
    if(lookup(parent, name, found, ino_out) == OK)
    {
        return EXIST;
    }
    ec->begin();
    if (ec->create(extent_protocol::T_FILE, ino_out) != extent_protocol::OK) {
       // printf("error creating file\n");
        return IOERR;
    }
    std::string buf;
    if (ec->get(parent, buf) != extent_protocol::OK) {
        //printf("error with get\n");
        return IOERR;
    }
    //size_t n = strlen(name);
    //buf = buf + itos(n) + std::string(name) + filename(ino_out);
    buf = buf + std::string(name) + "/";
    /*printf("\ncreate\n");
    printf(buf.c_str());
    printf("\n");
    printf(filename(ino_out).c_str());
    printf("\nfinisha\n");*/
    buf = buf + filename(ino_out) + "/";
    /* printf(buf.c_str());
    printf("\n");
    printf("\nino_out: %d\n", ino_out);
    printf("\nfinisha\n");*/
    if (ec->put(parent, buf) != extent_protocol::OK) {
        //printf("error with put\n");
        return IOERR;
    }
    ec->commit();
    return OK;
}

int chfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    /*
     * your lab2 code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    if (!isdir(parent)) {
        return IOERR;
    }
    bool found = false;
    if(lookup(parent, name, found, ino_out) == OK)
    {
        return EXIST;
    }
    ec->begin();
    if (ec->create(extent_protocol::T_DIR, ino_out) != extent_protocol::OK) {
        //printf("error creating dir\n");
        return IOERR;
    }
    std::string buf;
    if (ec->get(parent, buf) != extent_protocol::OK) {
        //printf("error with get\n");
        return IOERR;
    }
    //size_t n = strlen(name);
    //buf = buf + itos(n) + std::string(name) + filename(ino_out);
    buf = buf + std::string(name) + "/" + filename(ino_out) + "/";
    //printf("\nmakedir\n");
    //printf(buf.c_str());
    //printf("\nfinsih\n");
    if (ec->put(parent, buf) != extent_protocol::OK) {
      //  printf("error with put\n");
        return IOERR;
    }
    ec->commit();
    return OK;
}

int chfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    /*
     * your lab2 code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    if (!isdir(parent)) {
        return IOERR;
    }
    //ec->begin();
    std::string buf;
    if (ec->get(parent, buf) != extent_protocol::OK) {
        //printf("error with get\n");
        found = false;
        return IOERR;
    }
    //printf("lookup\n");
    //printf(buf.c_str());
    //printf("\nfinish\n");
    std::map<std::string,inum> list = stringSplit(buf,'/');
    //printf("lookup\n");
    //printf(buf.c_str());
    //printf("\nfinish\n");
    std::map<std::string,inum>::iterator it = list.find(std::string(name));
    if(it != list.end())
    {
        ino_out = it->second;
        found = true;
        //ec->commit();
        return OK;
    }
    found = false;
    return NOENT;
}

int chfs_client::readdir(inum dir, std::list<dirent> &list)
{
    /*
     * your lab2 code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    if (!isdir(dir)) {
        return IOERR;
    }
    //ec->begin();
    int r = OK;
    std::string buf;
    if(ec->get(dir,buf) != extent_protocol::OK)
    {
        r = IOERR;
        return r;
    }
    std::map<std::string,inum> clist = stringSplit(buf,'/');
    for(std::map<std::string,inum>::iterator it = clist.begin(); it != clist.end(); it++)
    {
        dirent tmp;
        tmp.inum = it->second;
        tmp.name = it->first;
        list.push_back(tmp);
    }
    //ec->commit();
    return r;
}

int chfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    /*
     * your lab2 code goes here.
     * note: read using ec->get().
     */
    std::string buf;
   // ec->begin();
    if (ec->get(ino, buf) != extent_protocol::OK) {
        //printf("error with get\n");
        return IOERR;
    }
    int len = buf.length();
    if (off >= len) {
        data = "";
        ec->commit();
        return OK;
    }
    data = buf.substr(off, size);
    //ec->commit();
    return OK;
}

int chfs_client::write(inum ino, size_t size, off_t off, const char *data, size_t &bytes_written)
{
    /*
     * your lab2 code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string buf;
    ec->begin();
    if (ec->get(ino, buf) != extent_protocol::OK) {
        //printf("error with get\n");
        return IOERR;
    }
    int len = buf.length();
    if (off+size >= len) {
        buf.resize(off+size);
    }
    for (size_t i=off; i<off+size; i++)
        buf[i] = data[i-off];

    if (ec->put(ino, buf) != extent_protocol::OK) {
        //printf("error with put\n");
        return IOERR;
    }
    ec->commit();
    return OK;
}

int chfs_client::unlink(inum parent,const char *name)
{
    /*
     * your lab2 code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    int r = OK;
    std::string buf;
    ec->begin();
    if(ec->get(parent,buf) != extent_protocol::OK)
    {
        r = IOERR;
        return r;
    }
    std::map<std::string,inum> list = stringSplit(buf,'/');
    std::map<std::string,inum>::iterator it = list.find(name);
    if(it == list.end())
    {
        r = NOENT;
        return r;
    }
    inum fi = it->second;
    if(ec->remove(fi) != extent_protocol::OK)
    {
        r = IOERR;
        return r;
    }
    list.erase(it);
    buf = "";
    for(it = list.begin(); it != list.end(); it++)
    {
        buf = buf + std::string(it->first) + "/" + filename(it->second) + "/";
    }
    if(ec->put(parent,buf) != extent_protocol::OK)
    {
        r = IOERR;
        return r;
    }
    ec->commit();
    return r;
}

int chfs_client::symlink(const char *link, inum parent, const char *name, inum &ino_out)
{
    printf("symlink\n");
    if (!isdir(parent)) {
        return IOERR;
    }
    ec->begin();
    bool found = false;
    if(lookup(parent, name, found, ino_out) == OK)
    {
        return EXIST;
    }
    if (ec->create(extent_protocol::T_SYM, ino_out) != extent_protocol::OK) {
        //printf("error creating file\n");
        return IOERR;
    }
    if (ec->put(ino_out, std::string(link)) != extent_protocol::OK) {
        //printf("error with put\n");
        return IOERR;
    }
    std::string buf;
    if (ec->get(parent, buf) != extent_protocol::OK) {
        //printf("error with get\n");
        return IOERR;
    }
    size_t n = strlen(name);
    buf = buf + std::string(name) + "/" + filename(ino_out) + "/";
    if (ec->put(parent, buf) != extent_protocol::OK) {
        //printf("error with put\n");
        return IOERR;
    }
    ec->commit();
    return OK;
}
