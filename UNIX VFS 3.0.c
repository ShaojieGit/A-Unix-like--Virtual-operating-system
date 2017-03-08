//磁盘初始化模块2.0（修改：加入用户进程表，存入数据区）(空闲i结点填充i结点区）
//用户登录
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define DIRNUM      128     //每个目录包含的最大项数
#define NHINO       128         //内存i结点哈希链表的大小（取余数法)
#define NOFILE      20      //用户进程打开表最大表项数
#define SYSOPENFILE 40    //最多允许打开文件数，系统打开文件最大表项数
#define USERNUM     10      //最多10个用户(进行文件操作？？？）
#define PWDNUM      32      //系统最大登录用户数
#define ROUTENUM    40      //最大路径深度
struct dinode{ //磁盘I结点（每个32个字节,这里占用其中28个字节)
    short inode_no;   //i结点号（内部标识符）(-1为空）
    short inode_type;  //i结点类型（-1为空结点，0为目录文件，1为文本文件）
    short i_mode;     //读写权限  (0为高级，管理员有所有权，1为低级，都可以读写）
    short file_size;    //文本的字节数（目录文件写成5了）
    short i_addr[10];    //物理地址索引表
};  //还需要一个类型字段标记是不是空的i结点

struct inode{    //内存i结点NICFREE
    struct inode *last;
    struct inode *next;
    //int user_sharenum;   //共享次数(打开用户数量）     在系统打开表中进行统计共享次数（读写次数）这就没用了
    //int disk_sharenum;    //磁盘共享次数，写快捷方式时使用
    char change_flag;    //修改标志    (0为未修改，1为修改）

    //----以下为从磁盘i结点复制过来的部分
    short inode_no;   //i结点号（内部标识符）
    short inode_type;  //i结点类型（-1为空结点，0为目录文件，1为索引文件，2为普通文件）
    int i_mode;     //读写权限   （0为高级，1为低级）
    int file_size;    //文件大小（多少块）
    int i_addr[10];    //物理地址索引表
};

struct direct{  //目录项文件（每个16B)
    short inode_no;   //i结点号（内存i结点)(2B)
    char file_name[14];   //文件名(14B)
};

struct filsys{   //超级快（管理快），能放在一个块里（超级块）(315个字节）
    //int inode_num;   //剩余磁盘i结点个数
    //int disk_size;     //物理磁盘块数   ????有毛用
    int freeblock_num;  //剩余空闲块块数
    int freeblock_stack_num;  //空闲块栈中空闲块数（将操作系统教程与实验p230组长块第一个表项移到这里
    int freeblock_stack[50];   //空闲块栈(成组链算法中每组块数为50
    int freeinode_stack_num;    //空闲i结点栈中空闲i结点数量
    short freeinode_stack[50];   //空闲i结点栈 (栈大小为50与磁盘i结点分配算法有关，参看我的文库《重庆大学操作系统教程P10）
    short flag_inode;  //铭记i结点，分配给下一组空闲i结点栈（作用：在磁盘i结点数组中最大（最后）一个i结点，决定空闲栈空时搜索空闲i结点的起始位置（参考上一行资料）(我认为铭记i结点不在当前空闲i结点栈中，而是下次搜索空闲i结点时作为第一个空闲块！？）
    char changed;  //超级快在内存是否被修改的标识
};

struct user_process{   //用户进程
    short user_id;  //用户进程编号
    short group_id;  //用户组号   0号为系统进程（管理员），1号为普通用户进程
    char password[12];  //口令
};


struct dir{   //目录文件   2048+4=2052B，5个数据块

    struct direct direct_list[DIRNUM];   //DIRNUM=128,每个目录包含的最大项数   16*128=2048B=4个数据块
    int size;  //目录文件表项数
};


struct hinode{   //哈希链表表项
		struct inode *i_forw;
};
struct hinode hinode_index[NHINO];   //内存i结点哈希链表索引表,NHINO=128, 内存i结点哈希链表的大小（取余数法)

struct system_file{  //系统打开文件表表项    《操作系统教程与实验》226
    char open_type;   //打开方式（读还是写）   0为写，1为读
    int read_count;  //读进程个数
    int write_count;   //写进程个数route_dinode
    short inode_no;     //i结点号，用来计算内存i结点哈希链表索引位置
    char filename_open[14];   //系统打开的文件名
    struct inode *f_inode; //打开文件内存i结点指针,通过在内存i结点哈希链表中搜索获得
    //int f_off;  //文件读写指针？？？？？？？？？
};
struct system_file sys_ofile[SYSOPENFILE];   //系统打开文件表SYSOPENFILE=40最多允许打开文件数，系统打开文件最大表项数

struct user_file{   //用户打开文件表表项
    //int default_mode;   //默认打开方式？？？？？？???????????
    int u_ofile[NOFILE];   //系统打开文件表的索引，NOFILE=20
    short user_id;  //用户进程编号
    short group_id;   //用户组号
    short route_stack[ROUTENUM];   //路径栈,存放目录文件的i结点号（栈顶为当前目录）   最多40层
    char dirname_stack[ROUTENUM][14];    //路径名栈，仅用于显示当前路径
    int route_stack_head;   //路径栈栈顶(以上两个栈共用一个栈指针)
};

struct index_block{   //物理索引块
    short index[256];
};
struct dirdetail
{
    short inode_no;
    char filename[14];
    short file_type;
};
struct dirdetail current_dirdetail[DIRNUM];
struct user_file user_file_list[USERNUM];  //当前已登录用户(最多10个）
struct filsys internal_filsys;   //内存超级块，每次启动时重写
struct block_grouphead{  //组长块
    short block_index[50];
};






int online(short userid);
int open(char *file_name,short userid);
short new_inode();
int insert_hinode(short inode_no,struct inode* inode_insert);
int get_dirdetail(int currentdir_inode_no) ;
int giveblock(short *addr,int num_block);
int release_block(short *addr);
int read_superblock();
int user_file_list_initialise();  //用户打开文件表表表的初始化
int sys_ofile_initialise() ;     //系统打开文件表初始化
int hinode_index_initialise();   //内存i结点哈希链表索引表的初始化

int initialise()   //初始化
{
    //函数描述：在程序中构建磁盘，将之保存为一个真正的文件，并保存
    int i;

    /*数据结构实现*/
    FILE *disk;    //磁盘模拟文件
    struct filsys myfilsys;



    /*建立存放磁盘的文件*/
    disk=fopen("disk","wb+");
    fseek(disk,279552,0);  //磁盘为(1+1+32+512)*512B
    fclose(disk);
    disk=NULL;
    disk=fopen("disk","rb+");//重新打开
    /*超级块的初始化*/
    //myfilsys.inode_num=511;   //初始时剩余511个磁盘i结点，因为有一个根目录的i结点
    //myfilsys.disk_size=512-5-1;
    myfilsys.freeblock_num=506;  //512-5-1（除去根目录文件占的5个块，用户表1个）
    myfilsys.freeblock_stack_num=7;
    for(i=0;i<7;i++)   //根据成组链分配算法计算出初始时最后一组空闲块为8块
        myfilsys.freeblock_stack[i]=40+i;   //第34-38块作为根目录文件，,第39块为用户表，空闲块从第40块开始
    for(i=7;i<50;i++)   //栈里的空索引
        myfilsys.freeblock_stack[i]=-1;
    myfilsys.freeinode_stack_num=50;
    for(i=0;i<50;i++)
        myfilsys.freeinode_stack[i]=i+1;  //初始的时候第一个i结点为根目录文件的i结点
    myfilsys.flag_inode=51;    //铭记i结点
    myfilsys.changed='n';
    /*filsys写入磁盘超级块*/
    fseek(disk,512,0);
    fwrite(&myfilsys,315,1,disk);
    /*fseek(disk,512,0);    //测试超级块是否写入成功
    struct filsys filsys_receive;
    fread(&filsys_receive,425,1,disk);
    printf(">>>>>%d",filsys_receive.flag_inode);
    fclose(disk);*/

    /*根目录文件i结点*/

    struct dinode rootdir_inode;
    rootdir_inode.inode_no=0;   //根目录文件i结点为第0个i结点
    rootdir_inode.inode_type=0;  //目录文件
    rootdir_inode.i_mode=1;  //根目录权限为低级
    rootdir_inode.file_size=5;  //目录文件大小为5块
    for(i=0;i<10;i++)
        rootdir_inode.i_addr[i]=-1;  //索引为空时赋给-1
    for(i=0;i<5;i++)    //根目录文件5个数据块的索引
        rootdir_inode.i_addr[i]=34+i;
    fseek(disk,1024,0);
    fwrite(&rootdir_inode,28,1,disk);

    /*i结点去填充空闲i结点*/
    struct dinode rootdir_inode_empty;
    rootdir_inode_empty.inode_type=-1;
    int pointer_i;  //填充空闲i结点时作为指针
    for(i=1;i<512;i++)
    {
        rootdir_inode_empty.inode_no=i;
        pointer_i=1056+(i-1)*32;
        fseek(disk,pointer_i,0);
        fwrite(&rootdir_inode_empty,28,1,disk);
    }


 /*  fseek(disk,1056,0);    //测试
    struct dinode rootdir_inode_receive;
    fread(&rootdir_inode_receive,28,1,disk);
    printf("\n>>>>>>>>>>>>>>>%d",rootdir_inode_receive.inode_type);
*/
    /*根目录文件*/
    /*struct dir{   //目录文件   2048+4=2052B，5个数据块
    struct direct direct[DIRNUM];   //DIRNUM=128,每个目录包含的最大项数   16*128=2048B=4个数据块
    int size;  //目录文件表项数
};*/
    struct dir rootdir;
    rootdir.size=0;
    for(i=0;i<DIRNUM;i++)
    {
        rootdir.direct_list[i].inode_no=-1;
        //rootdir.direct_list[i].
    }
    fseek(disk,17408,0);
    fwrite(&rootdir,2052,1,disk);
    /*
    int x;     //测试1
    short y;
    fseek(disk,19456,0);
    fread(&x,4,1,disk);
    printf("%d",x);
    return 0;*/
    /*struct dir rootdir_receive;   //测试2
    fseek(disk,17408,0);
    fread(&rootdir_receive,2052,1,disk);
    printf("%d",rootdir_receive.size);*/

    /*数据块初始化，成组链分配算法的初始化*/
    int block_index[50];
    int j=0;
    block_index[j++]=-1;   //倒数第二组组长块第一个索引为空
    for(i=545;i>=497;i--)
    {
        block_index[j++]=i;
    }   //第二组组长块完成
    //now,i=496
    /*struct block_grouphead{  //组长块    200B
    int block_index[50];
};*/
    struct block_grouphead second ;    //用于存储第二组组长块的结构体
    for(i=0;i<50;i++)
        second.block_index[i]=block_index[i];
    fseek(disk,253952,0);    //496*512=253952   第496块为第二组组长块
    fwrite(&second,200,1,disk);

    struct block_grouphead head_block;

    head_block.block_index[0]=496;
    int temp=0;
    for(i=495;i>=96;i--)   //i为最后一组组长块块号
    {
        temp++;
        if(temp==50)
        {
            //将head_block作为组长块写入当前块
            fseek(disk,i*512,0);
            fwrite(&head_block,200,1,disk);
            //当前块号作为下一个组长块的第一个索user_list_current_num引
            head_block.block_index[0]=i;
            temp=0;
        }
        else   //当前块号加入head_block.block_index[]
        {
            head_block.block_index[temp]=i;
        }
    }
    /*最后一组（最左侧一组的完成)*/
    for(i=0;i<50;i++)
        head_block.block_index[i]=96-i;
    fseek(disk,40*512,0);
    fwrite(&head_block,200,1,disk);

    /*fseek(disk,512*40,0);    //测试
    struct block_grouphead head_block_receive;
    fread(&head_block_receive,200,1,disk);
    for(i=0;i<50;i++)
        printf("%d ",head_block_receive.block_index[i]);*/

    /*磁盘静态用户表初始化*/
    /*
    struct user_process{   //用户进程    16B
    short user_id;  //用户进程编号
    short group_id;  //用户组号   0号为系统进程（管理员），1号为普通用户进程
    char password[12];  //口令
};*/
    struct user_process user_list[PWDNUM];  //磁盘静态用户表    16*32=512B
    user_list[0].user_id=0;   //0号用户
    user_list[0].group_id=0;   //管理员
    strcpy(user_list[0].password,"123");
    for(i=1;i<32;i++)
        user_list[i].user_id=-1;   //其余31个用户为空
    fseek(disk,39*512,0);
    fwrite(&user_list,512,1,disk);

    /*fseek(disk,39*512,0);    //测试
    struct user_process user_list_receive[PWDNUM];  //磁盘静态用户表    16*32=512B
    fread(&user_list_receive,512,1,disk);
    for(i=0;i<32;i++)
        printf("%d %d %s\n",user_list_receive[i].user_id,user_list_receive[i].group_id,user_list_receive[i].password);*/
    fclose(disk);
    disk=NULL;

    return 0;

}

int iget (short dinodeid,struct dinode *inode_return)
{
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    int inode_pointer=1024+32*dinodeid;
    fseek(disk,inode_pointer,0);
    fread(inode_return,28,1,disk);
    fclose(disk);
    disk=NULL;
    return 0;
}
int iput(short dinodeid,struct dinode *inode_write)
{
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    int inode_pointer=1024+32*dinodeid;
    fseek(disk,inode_pointer,0);
    fwrite(inode_write,28,1,disk);
    fclose(disk);
    disk=NULL;
    return 0;
}
int format()   //格式化
{
    initialise();
    read_superblock();
    user_file_list_initialise();  //用户打开文件表表表的初始化
    sys_ofile_initialise() ;     //系统打开文件表初始化
    hinode_index_initialise();   //内存i结点哈希链表索引表的初始化
    return 0;
}

int user_file_list_initialise()   //当前已登录用户进程表的初始化
{
    int i,j;
    for(i=0;i<USERNUM;i++)                         //初始化当前用户进程表
        {
            user_file_list[i].user_id=-1;
            for(j=0;j<NOFILE;j++)
                {
                    user_file_list[i].u_ofile[j]=-1;  //初始时用户没有打开文件
                    user_file_list[i].route_stack_head=0; //初始时路径栈为空
                }
        }
    return 0;
}
int sys_ofile_initialise()     //系统打开文件表的初始化
{
    int i;
    for(i=0;i<SYSOPENFILE;i++)
    {
        sys_ofile[i].read_count=0;
        sys_ofile[i].write_count=0;
        sys_ofile[i].inode_no=-1;
        sys_ofile[i].f_inode=NULL;  //内存i结点置空
        strcpy(sys_ofile[i].filename_open,"");
    }
    return 0;
}
int hinode_index_initialise()   //内存i结点哈希链表索引表的初始化
{
    int i;
    for(i=0;i<NHINO;i++)
        hinode_index[i].i_forw=NULL;
    return 0;
}
int adduser(short userid,short group_id,char *password,short father)
{
    int i;
    //------------
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件

    struct user_process user_list[PWDNUM];  //磁盘静态用户表读取    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);
    for(i=0;i<PWDNUM;i++)
        if(user_list[i].user_id==userid)
        {
            printf("用户进程ID冲突,添加失败！！！\n");
            return 0;
        }

    short father_groupid=-1;
    //判断父亲用户是否已登录
    int pos_father=online(father);
    if(pos_father==-1)
    {
        printf("父进程未登录！\n");
        return 0;
    }
    father_groupid=user_file_list[pos_father].group_id;
    if(father_groupid>group_id)   //普通用户创建管理员用户错误，这里的i和前面的i有关联
    {
        printf("权限不足！");
        return 0;
    }
    //-------------------------------判断是否存在ID相同的用户进程



/*    int father_flag=0;
    short father_groupid=-1;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==father)
            {
                father_flag=1;
                father_groupid=user_file_list[i].group_id;
            }
    if(father_flag==0)
    {
        printf("父进程未登录！\n");
        return 0;
    }

    //判断新建用户的级别是否与父进程吻合
    if(father_groupid>group_id)   //普通用户创建管理员用户错误，这里的i和前面的i有关联
    {
        printf("权限不足！");
        return 0;
    }
    */



    struct user_process user_tosave;
    user_tosave.user_id=userid;
    user_tosave.group_id=group_id;
    strcpy(user_tosave.password,password);
    for(i=0;i<PWDNUM;i++)
        if(user_list[i].user_id==-1)
        break;
    //计算存储位置
    int pos;
    pos=39*512+16*i;
    fseek(disk,pos,0);
    fwrite(&user_tosave,16,1,disk);   //存储该用户
    fclose(disk);
    return 0;
}
int login(char *param_userid,char *param_password)
{


    int i,j,k;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    short userid;
    //short groupid;
    char password[12];
    /*读取参数*/
    userid=short(atoi(param_userid));
    strcpy(password,param_password);

    struct user_process user_list[PWDNUM];  //磁盘静态用户表读取    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);

    //printf(">>>>>>>>>>>>>>>>%d  %s<<<<\n",user_list[1].user_id,user_list[1].group_id);  为什么加上这句就报错？？？？
    int login_flag=0;   //是否登录成功
    int repeat_login_flag=0;//是否重复登录


        for(j=0;j<10;j++)
        {
            if(userid==user_file_list[j].user_id)
            {
                printf("重复登录！！！\n");
                repeat_login_flag=1;
            }
        }
        if(repeat_login_flag==1)
            return 0;    //重复登录就退出登录函数，下次输入登录命令时再登录


        for(i=0;i<PWDNUM;i++)
            if(userid!=-1&&userid==user_list[i].user_id&&strcmp(password,user_list[i].password)==0)
            {   //口令正确


                for(j=0;j<10;j++)
                    if(user_file_list[j].user_id==-1)
                    {
                        //将该用户存储到当前用户表
                        user_file_list[j].user_id=userid;
                        user_file_list[j].group_id=user_list[i].group_id;
                        user_file_list[j].route_stack_head=0;
                        for(k=0;k<NOFILE;k++)
                            user_file_list[j].u_ofile[k]=-1;  //用户表到系统表索引初始化
                        //user_file_list[j].route_stack_head=0;
                        //打开根目录-----------------------------------------------------------------------------
                        int pos_userid;  //用户进程表在用户进程表表中的位置
                        char open_file_name[14];
                        strcpy(open_file_name,"root");
                        pos_userid=online(userid);




                        //用户文件表修改(此时可以确定可以打开根目录）
                        int stack_head=user_file_list[pos_userid].route_stack_head;
                        user_file_list[pos_userid].route_stack[stack_head]=0;   //根目录i结点压入栈顶
                        strcpy(user_file_list[pos_userid].dirname_stack[stack_head],"root");   //根目录名压入路径名栈
                        user_file_list[pos_userid].route_stack_head++;
                        //系统打开文件表修改
                        /*int sys_rootdir_open_flag=0;  //系统打开文件表是否已经打开根目录文件的标记
                        int pos_rootdir_sys_old;         //根目录文件在系统打开表中的位置
                        for(l=0;l<SYSOPENFILE;l++)   //判断系统打开文件表中有没有根目录文件的打开结点
                            {
                                if(sys_ofile[l].inode_no==0)
                                {
                                    sys_rootdir_open_flag=1;
                                    pos_rootdir_sys_old=i;
                                    //user_file_list[pos_userid].u_ofile
                                    for(k=0;k<NOFILE;k++)
                                        if(user_file_list[pos_userid].u_ofile[k]==-1)
                                            {
                                                user_file_list[pos_userid].u_ofile[k]=pos_rootdir_sys_old;  //写用户打开表中的索引
                                                break;
                                            }
                                    break;
                                }
                            }
                        if(sys_rootdir_open_flag==1)   //已经存在,填写用户打开表中的索引
                            sys_ofile[l].read_count++;   //读者加1
                        else  //系统当前没有打开根目录文件，现在打开
                        {
                            for(l=0;l<SYSOPENFILE;l++)
                            {


                                if(sys_ofile[l].inode_no==-1)   //系统打开表打开根目录文件
                                {
                                    //填入用户打开文件表
                                    for(k=0;k<NOFILE;k++)
                                    if(user_file_list[pos_userid].u_ofile[k]==-1)
                                    {
                                        user_file_list[pos_userid].u_ofile[k]=i;  //写用户打开表中的索引
                                        break;
                                    }

                                    sys_ofile[l].inode_no=0;
                                    sys_ofile[l].write_count=0;
                                    sys_ofile[l].read_count=1;
                                    sys_ofile[l].open_type=1;  //方式为读
                                    //分配内存i结点
                                    struct inode *inode_rootdir;
                                    inode_rootdir=(struct inode*)malloc(sizeof(struct inode));
                                    inode_rootdir->last=NULL;
                                    inode_rootdir->next=NULL;
                                    inode_rootdir->change_flag='n';

                                    inode_rootdir->file_size=5;  //以下为磁盘的根目录文件i结点(直接复制初始化时的根目录即可）
                                    inode_rootdir->inode_no=0;
                                    inode_rootdir->inode_type=0;
                                    inode_rootdir->i_mode=1;
                                    for(k=0;k<10;k++)
                                        inode_rootdir->i_addr[k]=-1;  //索引为空时赋给-1
                                    for(k=0;k<5;k++)    //根目录文件5个数据块的索引
                                        inode_rootdir->i_addr[k]=34+i;
                                    //插入内存i结点链表
                                    insert_hinode(0,inode_rootdir);
                                    sys_ofile[l].f_inode=inode_rootdir;
                                    break;
                                }
                            }
                        }*/
                        //根目录打开完毕----------------------------------------------------------------------------------------
                        //open(root,userid);
                        break;
                    }

                if(j==10)
                {
                    printf("已经达到当前最大用户进程数（10），登录失败！");
                    return 0;
                }
                printf("登录成功！\n");
                login_flag=1;
                break;
            }
        if(login_flag==0)
            printf("登录失败\n");


    fclose(disk);                             //         为什么加上这句就报错
   /* for(i=0;i<10;i++)
        printf("(%d,%d)  ",user_file_list[i].user_id,user_file_list[i].group_id);*/
    return 0;
}

int logout(short userid)
{
    int i;
    //关闭打开文件
    //判断用户是否登录
    int already_login_flag=0;
    int pos_user;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==userid)
            {
                already_login_flag=1;
                pos_user=i;
                break;
            }
    if(already_login_flag==0)
    {
        printf("用户未登录！！！\n");
        return 0;
    }
    int num_samename=0;
    for(i=0;i<NOFILE;i++)   //用户
    {//user_file_list[USERNUM]
        int index_sys=user_file_list[pos_user].u_ofile[i];   //用户打开文件在系统表中的索引位置
        if(sys_ofile[index_sys].inode_no!=-1)   //打开了该文件
        {
            int file_inode_no=sys_ofile[i].inode_no;   //获得文件i结点(删除时释放i结点物理块有用）
            //修改系统打开表

            if(sys_ofile[index_sys].read_count+sys_ofile[index_sys].write_count==1)  //只有一个用户打开
            {

                strcpy(sys_ofile[index_sys].filename_open,"");
                sys_ofile[index_sys].read_count=0;
                sys_ofile[index_sys].read_count=0;
                sys_ofile[index_sys].inode_no=-1;

                //释放内存i结点
                if(hinode_index[file_inode_no%128].i_forw==sys_ofile[index_sys].f_inode)  //哈希链表对应索引位置第一个i结点为当前内存i结点
                    hinode_index[file_inode_no%128].i_forw=NULL;
                else
                {
                    sys_ofile[index_sys].f_inode->last->next=sys_ofile[index_sys].f_inode->next;
                    if(sys_ofile[index_sys].f_inode->next!=NULL)
                        sys_ofile[index_sys].f_inode->next->last=sys_ofile[index_sys].f_inode->last;
                }
            }
            else   //有多个用户进程在读
                sys_ofile[index_sys].read_count--;

            //修改用户表
            user_file_list[pos_user].u_ofile[i]=-1;
        }
        num_samename++;

    }

    //用户登出
    user_file_list[pos_user].user_id=-1;

    return 0;
}
int deleteuser(short delete_who,short who_delete)
{
    int i;
    //删除者是否在线
    int already_login_flag=0;
    int pos_who_delete;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==who_delete)
            {
                already_login_flag=1;
                pos_who_delete=i;
                break;
            }
    if(already_login_flag==0)
    {
        printf("删除者未登录!!!\n");
        return 0;
    }
    //删除者必须是管理员
    else
    {
        if(user_file_list[pos_who_delete].group_id!=0)   //不是管理员
            {
                printf("删除者不是管理员，没有删除权限！！！\n");
                return 0;
            }
    }
    //考虑被删除者是否在线
    already_login_flag=0;
    int pos_user;   //用户文件表表的位置
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==delete_who)
            {
                already_login_flag=1;
                pos_user=i;
                break;
            }

    //在线则关闭打开文件
    if(already_login_flag==1)   //要删除的
    {
        //关闭打开文件   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>待完成
        //被删除用户下线
        user_file_list[pos_user].user_id=-1;
    }
    //删除
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    struct user_process user_list[PWDNUM];  //磁盘静态用户表读取    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);
    int exist_flag=0;
    int pos_delete_who;
    for(i=0;i<PWDNUM;i++)   //被删除者是否存在
        if(user_list[i].user_id==delete_who)
            {
                exist_flag=1;
                pos_delete_who=i;
                break;
            }
    if(exist_flag==0)
    {
        printf("被删除者不存在！！！\n");
        return 0;
    }
    int offset_disk=39*512+16*pos_delete_who;
    fseek(disk,offset_disk,0);
    user_process empty_user_process;
    empty_user_process.user_id=-1;
    fwrite(&empty_user_process,16,1,disk);
    fclose(disk);
    return 0;
}
int showalluser() //显示磁盘中的用户表
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件

    struct user_process user_list[PWDNUM];  //磁盘静态用户表读取    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);
    for(i=0;i<PWDNUM;i++)
        if(user_list[i].user_id!=-1)
        printf("ID=%d GROUP=%d password=%s\n",user_list[i].user_id,user_list[i].group_id,user_list[i].password);
    return 0;
}
int showonlineuser()   //显示在线用户列表
{
    int i,count=0;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id!=-1)
            printf("在线用户%d userid=%d\n",++count,user_file_list[i].user_id);
    return 0;
}

int online(short userid)   //判断用户是否在线，在线则返回用户打开文件表表位置,不在线返回-1
{
    int i;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==userid)
                return i;     //已登录
        return -1;            //未登录
}
int open(char *file_name,short userid)
{   //打开文件
    /*
    函数说明：打开根目录包含了初始化用户目录的内容，与打开其他文件分开写
                目录文件只能以读的方式打开
                打开目录文件不用维护修改系统表和内存i结点
    */
    int i,j;
    FILE *disk;   //读取该目录文件的i结点---------------------------------
    disk=fopen("disk","rb+");
    int pos_userid;  //用户进程表在用户进程表表中的位置
    int repeat_open_flag;  //重复打开标记
    char open_file_name[14];
    strcpy(open_file_name,file_name);
    pos_userid=online(userid);
    if(pos_userid==-1)
    {
        printf("用户未登录\n");
        return 0;
    }
    //在当前目录下搜索该文件名，找到就获取i结点号，为判断是否重复打开文件做准备；找不到就报错
    int current_dir_inode_no;//当前目录i结点
    current_dir_inode_no=user_file_list[pos_userid].route_stack[user_file_list[pos_userid].route_stack_head-1];   //当前用户路径栈栈栈顶为当前目录
    get_dirdetail(current_dir_inode_no);   //获取该用户当前目录详细信息//----bug_key2
    int file_type;    //要打开文件的类型
    int file_inode_no;   //要打开文件的i结点号
    for(i=0;i<DIRNUM;i++)
    {    //根据文件名在当前目录中寻找
        if(current_dirdetail[i].inode_no!=-1&&(strcmp(current_dirdetail[i].filename,file_name)==0))
            {
                file_type=current_dirdetail[i].file_type;    //--------------   x=y  -1
                file_inode_no=current_dirdetail[i].inode_no;
                break;
            }
    }
    if(i==DIRNUM)//没有找到
    {
        printf("文件不存在!!!\n");
        return 0;
    }
    //判断是否重复打开
    repeat_open_flag=0;
    int temp;
    for(i=0;i<NOFILE;i++)
        if((temp=user_file_list[pos_userid].u_ofile[i])!=-1)
            if(sys_ofile[temp].inode_no==file_inode_no)
                {
                    repeat_open_flag=1;
                    break;
                }

    if(repeat_open_flag==1)
    {
        printf("重复打开了文件或目录！！！\n");                //测试点1，未测试
        return 0;
    }

    //是目录文件、文本文件两种情况进行打开
    if(file_type==0)   //是目录文件,打开目录
    {  //以读的方式打开
        //用户文件表(关于路径存储信息）修改
        int stack_head=user_file_list[pos_userid].route_stack_head;
        user_file_list[pos_userid].route_stack[stack_head]=file_inode_no;   //目录i结点压入栈顶
        strcpy(user_file_list[pos_userid].dirname_stack[stack_head],file_name);   //目录名压入路径名栈
        user_file_list[pos_userid].route_stack_head++;
        //------------------------------测试------------------
    //printf("%d->%d %d",user_file_list[pos_userid].route_stack_head,user_file_list[pos_userid].route_stack[0],user_file_list[pos_userid].route_stack[1]);

    }
    else    //是文本文件,打开文本,读取文本
    {
        //读写方式选择,考虑读写权限问题，change_flag，重新释放分配物理空间
        //默认以读的方式打开
        //修改打开方式为写后保持写文件状态
        struct dinode file_dinode;

        fseek(disk,1024+32*file_inode_no,0);
        fread(&file_dinode,28,1,disk);
        int file_level=file_dinode.i_mode;   //文件级别
        int user_level=user_file_list[pos_userid].group_id;   //用户级别
        //printf("file_dinode.id=%d user_level=%d  file_level=%d\n",file_dinode.inode_no,user_level,file_level);
        if(user_level==1&&file_level==0)
        {
            printf("管理员所有文件，禁止非管理员打开!!!\n");
            fclose(disk);
            return 0;
        }
        int sys_file_open_flag=0;  //系统打开文件表是否已经打开文件的标记
        int pos_file_sys_old;         //该文件在系统打开表中的位置
        for(i=0;i<SYSOPENFILE;i++)   //判断系统打开文件表中有没有目录文件的打开结点
        {
            if(sys_ofile[i].inode_no==file_inode_no)
            {   //系统已打开该目录（其他用户进程）
                sys_file_open_flag=1;
                pos_file_sys_old=i;
                //user_file_list[pos_userid].u_ofile
                for(j=0;j<NOFILE;j++)
                    if(user_file_list[pos_userid].u_ofile[j]==-1)
                        {
                            user_file_list[pos_userid].u_ofile[j]=pos_file_sys_old;  //写用户打开表中的索引
                            break;
                        }
                if(j==NOFILE)
                {
                    printf("用户打开文件数达到上限！！！\n");
                    return 0;
                }
                break;
            }
        }
        int current_sys_pos=i;   //系统打开表中的位置（如果系统打开了）
        printf("读还是写？（w/r）:");
        char x[5];
        while(1)
        {
            scanf("%s",x);
            if(strcmp(x,"r")==0)
            {   //读文件
                if(sys_file_open_flag==1)   //系统已经打开
                {
                    printf("现在有%d个用户在读\n",sys_ofile[current_sys_pos].read_count);
                    printf("现在有%d个用户在写\n",sys_ofile[current_sys_pos].write_count);
                    if(sys_ofile[current_sys_pos].write_count>0)
                    {
                        printf("有用户进程在写，不能以读的方式打开！！！\n");
                        //恢复用户打开表打开状态
                        user_file_list[pos_userid].u_ofile[j]=-1;    //这里用到了前面一个循环中的量j
                        //退出
                        return 0;
                    }
                    else
                    {
                        //增加系统表中的读计数
                        sys_ofile[current_sys_pos].read_count++;

                        //下面可以开始读了!!!!!!!!!
                        if(file_dinode.file_size==0)
                        {
                            printf("1.这是一个空文件。\n");
                        }
                        else   //不是空文件
                        {
                            char *file_content;    //存储读取内容的位置
                            file_content=(char *)malloc(sizeof(char)*file_dinode.file_size+1);

                            int block_num=file_dinode.file_size/512+1;  //存储该文件用了几个块
                            int yushu=file_dinode.file_size%512;
                            if(yushu==0)
                                block_num++;   //文件占用物理块数


                            int *block_no;   //存储物理块号
                            block_no=(int *)malloc(sizeof(int)*block_num);
                            for(j=0;j<block_num;j++)
                            {
                                //去物理地址挨个读取物理块块号
                                if(j<9)
                                    block_no[j]=file_dinode.i_addr[j];
                                else   //读取一级索引块
                                {
                                    struct index_block this_index_block;
                                    if(j==10)
                                    {
                                        int index_pos=file_dinode.i_addr[9];
                                        fseek(disk,index_pos*512,0);
                                        fread(&this_index_block,512,1,disk);
                                        block_no[j]=this_index_block.index[j-9];
                                    }
                                    block_no[j]=this_index_block.index[j-9];
                                }
                            }
                            int temp_size=file_dinode.file_size;   //剩余读取字节数
                            int p_string=0;
                            for(j=0;j<block_num-1;j++)  //从物理块取到字符串里
                            {
                                fseek(disk,block_no[j]*512,0);
                                fread(&file_content[p_string],512,1,disk);
                                p_string+=512;
                                temp_size-=512;
                            }
                            fseek(disk,block_no[block_num-1]*512,0);
                            fread(&file_content[p_string],temp_size,1,disk);   //???????????????

                            file_content[file_dinode.file_size]='\0';
                            printf("%s\n",file_content);
                        }
                    }

                }
                else    //系统还没有打开,  现在打开
                {
                    //系统打开该文件
                    for(i=0;i<SYSOPENFILE;i++)
                    {
                        if(sys_ofile[i].inode_no==-1)   //系统打开表打开目录文件
                        {
                            //填入用户打开文件表
                            for(j=0;j<NOFILE;j++)
                            if(user_file_list[pos_userid].u_ofile[j]==-1)
                            {
                                user_file_list[pos_userid].u_ofile[j]=i;  //写用户打开表中的索引
                                break;
                            }

                            sys_ofile[i].inode_no=file_inode_no;
                            sys_ofile[i].write_count=0;
                            sys_ofile[i].read_count=1;
                            sys_ofile[i].open_type=1;  //方式为读
                            strcpy(sys_ofile[i].filename_open,file_name);
                            //分配内存i结点
                            struct dinode dinode_file;

                            fseek(disk,1024+32*file_inode_no,0);
                            fread(&dinode_file,28,1,disk);
                            struct inode *inode_file;
                            inode_file=(struct inode*)malloc(sizeof(struct inode));
                            inode_file->last=NULL;
                            inode_file->next=NULL;
                            inode_file->change_flag='n';

                            inode_file->file_size=dinode_file.file_size;  //以下为磁盘的目录文件i结点(默认属性(除了i结点号））
                            inode_file->inode_no=file_inode_no;
                            inode_file->inode_type=0;
                            inode_file->i_mode=dinode_file.i_mode;   //权限为低级
                            for(j=0;j<10;j++)
                                inode_file->i_addr[i]=dinode_file.i_addr[j];  //索引为空时赋给-1
                            //插入内存i结点哈希链表

                            insert_hinode(file_inode_no,inode_file);
                            sys_ofile[i].f_inode=inode_file;
                            break;
                        }
                    }
                    if(i==SYSOPENFILE)
                    {
                        printf("系统打开文件数达到上限！！！\n");
                        return 0;
                    }
                    //下面可以开始读了!!!!!!!!!
                    //printf("(%d 号i结点size=%d\n",file_dinode.inode_no,file_dinode.file_size);
                    if(file_dinode.file_size==0)
                    {
                        printf("2.这是一个空文件。\n");   //>>>>>>>>>>>>>>>>>>>
                    }
                    else   //不是空文件
                    {
                        char *file_content;    //存储读取内容的位置
                        file_content=(char *)malloc(sizeof(char)*file_dinode.file_size+1);

                        int block_num=file_dinode.file_size/512+1;  //存储该文件用了几个块
                        int yushu=file_dinode.file_size%512;
                        if(yushu==0)
                            block_num++;   //文件占用物理块数


                        int *block_no;   //存储物理块号
                        block_no=(int *)malloc(sizeof(int)*block_num);
                        for(j=0;j<block_num;j++)
                        {
                            //去物理地址挨个读取物理块块号
                            if(j<9)
                                block_no[j]=file_dinode.i_addr[j];
                            else   //读取一级索引块
                            {
                                struct index_block this_index_block;
                                if(j==10)
                                {
                                    int index_pos=file_dinode.i_addr[9];
                                    fseek(disk,index_pos*512,0);
                                    fread(&this_index_block,512,1,disk);
                                    block_no[j]=this_index_block.index[j-9];
                                }
                                block_no[j]=this_index_block.index[j-9];
                            }
                        }
                        int temp_size=file_dinode.file_size;   //剩余读取字节数
                        int p_string=0;
                        for(j=0;j<block_num-1;j++)  //从物理块取到字符串里
                        {
                            fseek(disk,block_no[j]*512,0);
                            fread(&file_content[p_string],512,1,disk);
                            p_string+=512;
                            temp_size-=512;
                        }
                        fseek(disk,block_no[block_num-1]*512,0);
                        fread(&file_content[p_string],temp_size,1,disk);   //???????????????

                        file_content[file_dinode.file_size]='\0';
                        printf("%s\n",file_content);
                    }
                }


                break;

            }

            else if(strcmp(x,"w")==0)
            {
                //写文件
                if(sys_file_open_flag==1)   //系统已经打开
                {
                    printf("现在有%d个用户在写\n",sys_ofile[current_sys_pos].write_count);
                    printf("现在有%d个用户在读\n",sys_ofile[current_sys_pos].read_count);
                    if(sys_ofile[current_sys_pos].read_count>0||sys_ofile[current_sys_pos].write_count>0)
                    {
                        printf("有用户进程在读或者写，不能以写的方式打开！！！\n");
                        //恢复用户打开表打开状态
                        user_file_list[pos_userid].u_ofile[j]=-1;    //这里用到了前面一个循环中的量j
                        //退出
                        return 0;
                    }
                    else
                    {
                        //在系统打开表注册

                        sys_ofile[current_sys_pos].write_count++;

                        //下面可以开始写了!!!!!!!!!
                        //释放物理块

                        //-------------------
                        //找块号
                      /*  int block_num=file_dinode.file_size/512+1;  //存储该文件用了几个块
                        int yushu=file_dinode.file_size%512;
                        if(yushu==0)
                            block_num++;   //文件占用物理块数


                        int *block_no;   //存储物理块号
                        block_no=(int *)malloc(sizeof(int)*block_num);
                        for(j=0;j<block_num;j++)
                        {
                            //去物理地址挨个读取物理块块号
                            if(j<9)
                                block_no[j]=file_dinode.i_addr[j];
                            else   //读取一级索引块
                            {
                                struct index_block this_index_block;
                                if(j==10)
                                {
                                    int index_pos=file_dinode.i_addr[9];
                                    fseek(disk,index_pos*512,0);
                                    fread(&this_index_block,512,1,disk);
                                    block_no[j]=this_index_block[j-9];
                                }
                                block_no[j]=this_index_block[j-9];
                            }
                        }
                        for(j=0;j<block_num;j++)
                        {
                            release_block(block_no[j]);
                        }*/
                        release_block(file_dinode.i_addr);  //addr在该函数中置空

                        //------------------
                        printf("输入文本内容：");
                        char file_content_write[1000];  //要输入的文本
                        scanf("%s",file_content_write);
                    //gets(file_content_write);
                        int file_size=strlen(file_content_write);   //文件字节数
                        file_dinode.file_size=file_size;

                        int block_num=file_size/512+1;  //存储该文件用几个块
                        int yushu=file_size%512;
                        if(yushu==0)
                            block_num++;   //文件占用物理块数
                        giveblock(file_dinode.i_addr,block_num);
                        //获取块号
                        int *block_no;   //存储物理块号
                        block_no=(int *)malloc(sizeof(int)*block_num);
                        for(j=0;j<block_num;j++)
                        {
                            //去物理地址挨个读取物理块块号
                            if(j<9)
                                block_no[j]=file_dinode.i_addr[j];
                            else   //读取一级索引块
                            {
                                struct index_block this_index_block;
                                if(j==10)
                                {
                                    int index_pos=file_dinode.i_addr[9];
                                    fseek(disk,index_pos*512,0);
                                    fread(&this_index_block,512,1,disk);
                                    block_no[j]=this_index_block.index[j-9];
                                }
                                block_no[j]=this_index_block.index[j-9];
                            }
                        }
                        //写入磁盘
                        int temp_size=file_dinode.file_size;   //剩余写入字节数
                        int p_string=0;
                        for(j=0;j<block_num-1;j++)  //从物理块取到字符串里
                        {
                            fseek(disk,block_no[j]*512,0);
                            fwrite(&file_content_write[p_string],512,1,disk);
                            p_string+=512;
                            temp_size-=512;
                        }
                        fseek(disk,block_no[block_num-1]*512,0);
                        fwrite(&file_content_write[p_string],temp_size,1,disk);   //???????????????
                        printf("1.写入成功\n");
                    }
                }
                else     //系统还没有打开
                {
                    //系统打开该文件
                    for(i=0;i<SYSOPENFILE;i++)
                    {
                        if(sys_ofile[i].inode_no==-1)   //系统打开表打开目录文件
                        {
                            //填入用户打开文件表
                            for(j=0;j<NOFILE;j++)
                            if(user_file_list[pos_userid].u_ofile[j]==-1)
                            {
                                user_file_list[pos_userid].u_ofile[j]=i;  //写用户打开表中的索引
                                break;
                            }

                            sys_ofile[i].inode_no=file_inode_no;
                            sys_ofile[i].write_count=1;
                            sys_ofile[i].read_count=0;
                            sys_ofile[i].open_type=0;  //方式为写
                            strcpy(sys_ofile[i].filename_open,file_name);
                            //分配内存i结点
                            struct dinode dinode_file;

                            fseek(disk,1024+32*file_inode_no,0);
                            fread(&dinode_file,28,1,disk);
                            struct inode *inode_file;
                            inode_file=(struct inode*)malloc(sizeof(struct inode));
                            inode_file->last=NULL;
                            inode_file->next=NULL;
                            inode_file->change_flag='n';

                            inode_file->file_size=dinode_file.file_size;  //以下为磁盘的目录文件i结点(默认属性(除了i结点号））
                            inode_file->inode_no=file_inode_no;
                            inode_file->inode_type=0;
                            inode_file->i_mode=dinode_file.i_mode;
                            for(j=0;j<10;j++)
                                inode_file->i_addr[i]=dinode_file.i_addr[j];
                            //插入内存i结点哈希链表

                            insert_hinode(file_inode_no,inode_file);
                            sys_ofile[i].f_inode=inode_file;
                            break;
                        }
                    }
                    if(i==SYSOPENFILE)
                    {
                        printf("系统打开文件数达到上限！！！\n");
                        return 0;
                    }
                    //下面可以开始写了!!!!!!!!!
                    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                    //下面可以开始写了!!!!!!!!!
                    //释放物理块

                    //-------------------
                    //找块号
               /*     int block_num=file_dinode.file_size/512+1;  //存储该文件用了几个块
                    int yushu=file_dinode.file_size%512;
                    if(yushu==0)
                        block_num++;   //文件占用物理块数


                    int *block_no;   //存储物理块号
                    block_no=(int *)malloc(sizeof(int)*block_num);
                    for(j=0;j<block_num;j++)
                    {
                        //去物理地址挨个读取物理块块号
                        if(j<9)
                            block_no[j]=file_dinode.i_addr[j];
                        else   //读取一级索引块
                        {
                            struct index_block this_index_block;
                            if(j==10)
                            {
                                int index_pos=file_dinode.i_addr[9];
                                fseek(disk,index_pos*512,0);
                                fread(&this_index_block,512,1,disk);
                                block_no[j]=this_index_block[j-9];
                            }
                            block_no[j]=this_index_block[j-9];
                        }
                    }
                    for(j=0;j<block_num;j++)
                    {
                        release_block(block_no[j]);
                    }*/
                    release_block(file_dinode.i_addr);  //addr在该函数中置空

                    //------------------
                    printf("输入文本内容：");
                    char file_content_write[1000];  //要输入的文本
                    scanf("%s",file_content_write);
                    //gets(file_content_write);
                    int file_size=strlen(file_content_write);   //文件字节数
                    file_dinode.file_size=file_size;

                    printf("(%d 号i结点size=%d\n",file_dinode.inode_no,file_dinode.file_size);
                    int block_num=file_size/512+1;  //存储该文件用几个块
                    int yushu=file_size%512;
                    if(yushu==0)
                        block_num++;   //文件占用物理块数
                    giveblock(file_dinode.i_addr,block_num);
                    //获取块号
                    int *block_no;   //存储物理块号
                    block_no=(int *)malloc(sizeof(int)*block_num);
                    for(j=0;j<block_num;j++)
                    {
                        //去物理地址挨个读取物理块块号
                        if(j<9)
                            block_no[j]=file_dinode.i_addr[j];
                        else   //读取一级索引块
                        {
                            struct index_block this_index_block;
                            if(j==10)
                            {
                                int index_pos=file_dinode.i_addr[9];
                                fseek(disk,index_pos*512,0);
                                fread(&this_index_block,512,1,disk);
                                block_no[j]=this_index_block.index[j-9];
                            }
                            block_no[j]=this_index_block.index[j-9];
                        }
                    }
                    //写入磁盘
                    int temp_size=file_dinode.file_size;   //剩余写入字节数
                    int p_string=0;
                    for(j=0;j<block_num-1;j++)  //从物理块取到字符串里
                    {
                        fseek(disk,block_no[j]*512,0);
                        fwrite(&file_content_write[p_string],512,1,disk);
                        p_string+=512;
                        temp_size-=512;
                    }
                    fseek(disk,block_no[block_num-1]*512,0);
                    fwrite(&file_content_write[p_string],temp_size,1,disk);   //???????????????
                    printf("2.写入成功\n");
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    //i结点写回
                    fseek(disk,file_dinode.inode_no*32+1024,0);

                    fwrite(&file_dinode,28,1,disk);


                }  //系统没打开时写完毕
                break;   //不需要修改当前目录
            }
            else
            {

                printf("输入w/r，重新输入：\n");
            }
        }
        getchar();

    }

    fclose(disk);
    return 0;
}

int insert_hinode(short inode_no,struct inode* inode_insert)   //内存i结点哈希链表的插入
{
     int insert_pos;   //在哈希链表中的插入位置
     insert_pos=inode_no%128;  //哈希函数

     if(hinode_index[insert_pos].i_forw!=NULL)  //该索引位置不为空
     {
            struct inode* x=hinode_index[insert_pos].i_forw;
            while(x->next!=NULL)
                x=x->next;
            x->next=inode_insert;   //此时x已经指向最后一个
            (x->next)->last=x;
    }
    else    //该索引位置为空
    {
            hinode_index[insert_pos].i_forw=inode_insert;
    }
    return 0;
}


int showroute(short userid)
{
    //user_file_list[USERNUM]
    int i;
    int pos=online(userid);
    if(pos==-1)
    {
        printf("用户未登录!!!\n");
        return 0;
    }
    for(i=0;i<user_file_list[pos].route_stack_head;i++)   //对栈中每一个userid取文件名
        printf("/%s",user_file_list[pos].dirname_stack[i]);
    printf("\n");
    return 0;
}

int giveblock_single()  //分配单个物理块（为索引块分配时使用）
{
    int block_return;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    if(internal_filsys.freeblock_stack_num>1)  //当前栈有大于1个空闲块
    {
        block_return=internal_filsys.freeblock_stack[--internal_filsys.freeblock_stack_num];   //获得栈顶块号
        internal_filsys.changed='y';
    }
    else
    {
        int block_manage;//组长块块号
        //if(internal_filsys.freeblock_stack[0]!=-1)既然主调函数中对空闲块数做出保证就不需要这个判断了

        block_manage=internal_filsys.freeblock_stack[0];
        //读取组长块到空闲块栈
        int offset_block_manage=512*block_manage;
        fseek(disk,offset_block_manage,0);
        fread(&internal_filsys.freeblock_stack[0],200,1,disk);   //改错
        internal_filsys.changed='y';
        internal_filsys.freeblock_stack_num=50;  //改错
        block_return=block_manage;
    }
    fclose(disk);
    return block_return;
}
int release_block_single(int block_no)
{   //释放单个块
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    if(internal_filsys.freeblock_stack_num<50)  //当前栈没满
        internal_filsys.freeblock_stack[internal_filsys.freeblock_stack_num++];  //压栈
    else
    {
        //释放的块写入组长块
        struct block_grouphead  group_head;
        for(i=0;i<50;i++)
            group_head.block_index[i]=internal_filsys.freeblock_stack[i];

        fseek(disk,block_no*512,0);
        fwrite(&group_head,100,1,disk);


        //超级块改写
        internal_filsys.freeblock_stack[0]=block_no;  //要释放的块作为组长块
        for(i=1;i<50;i++)
            internal_filsys.freeblock_stack[i]=-1;

        internal_filsys.freeblock_stack_num=0;
        internal_filsys.freeblock_num--;
    }
    internal_filsys.changed='y';
    fclose(disk);
    return 0;
}
int release_block(short *addr)
{    //释放块(需要保证file_
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    int need_release_block[265];  //暂存需要释放的块号
    int need_release_size=0;   //需要释放的块数
    int more_flag=0;
    for(i=0;i<9;i++)
    {
        if(addr[i]==-1)
            break;
        need_release_block[need_release_size++]=addr[i];
        addr[i]=-1;
    }
    if(more_flag==1)
    {
        if(addr[9]==-1)
        {
            //索引块索引为空，说明已经释放完毕
        }
        else
        {
            struct block_grouphead this_indexblock;  //索引块
            int index_block_no=addr[9];
            addr[9]=-1;
            fseek(disk,index_block_no*512,0);
            fread(&this_indexblock,100,1,disk);
            for(i=0;i<256;i++)
            {
                if(this_indexblock.block_index[i]!=-1)
                {
                    need_release_block[need_release_size++]=this_indexblock.block_index[i];
                }
                else
                    break;
            }
        }
    }
    for(i=0;i<need_release_size;i++)
        release_block_single(need_release_block[i]);
    return 0;
}
int giveblock(short *addr,int num_block)
{   //为文件分配磁盘块   （使用一次间接索引，就是addr[0]-addr[8]存放直接索引，addr[9]存放一次间接索引表
    int i,j;
    int real_block_num=num_block;  //真正分配的块数（包括索引块）
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    if(num_block>9)
        real_block_num++;
    if(num_block==0)
    {    //不分配块
        for(i=0;i<10;i++)
            addr[i]=-1;
        return 0;
    }
    if(real_block_num>internal_filsys.freeblock_num)    //要分配的块数大于当前剩余总块数
    {
        printf("剩余数据块不足！！！\n");
        return 0;
    }
    if(num_block>265)    //要分配的块数大于最大索引块数
    {
        printf("要分配的块数大于最大索引块数！！！\n");
        return 0;
    }

    //先获取分配的磁盘块号

    short block_no[511];
    for(i=0;i<num_block;i++)
    {

        if(internal_filsys.freeblock_stack_num>1)  //当前栈有大于1个空闲块
        {
            block_no[i]=internal_filsys.freeblock_stack[--internal_filsys.freeblock_stack_num];   //获得栈顶块号
            internal_filsys.changed='y';
        }
        else  //internal_filsys.freeblock_stack_num=1;
        {
            int block_manage;//组长块块号
            if(internal_filsys.freeblock_stack[0]!=-1)
            {
                block_manage=internal_filsys.freeblock_stack[0];
                //读取组长块到空闲块栈
                int offset_block_manage=512*block_manage;
                fseek(disk,offset_block_manage,0);
                fread(&internal_filsys.freeblock_stack[0],200,1,disk);   //改错
                internal_filsys.changed='y';
                internal_filsys.freeblock_stack_num=50;  //改错
                block_no[i]=block_manage;
            }
            else
            {   //之前已经进行了判断，这段不会执行，标准算法是没有之前对于块够不够的判断的，但是到这时会将进程阻塞，但是我们的程序没有进城阻塞机制，因此提前判断块够不够
                printf("剩余数据块不足！！！\n");
                return 0;
            }

        }
    }
    internal_filsys.freeblock_num-=num_block;
    //把磁盘块号填入i结点物理索引表
    if(num_block<10)  //小于十块不用多级索引
    {
        for(i=0;i<num_block;i++)
            addr[i]=block_no[i];
        for(;i<10;i++)
            addr[i]=-1;
    }
    else
    {
        for(i=0;i<9;i++)
            addr[i]=block_no[i];
        //分配一级索引块
        int index_block_no=giveblock_single();
        struct index_block index_block_current;
        for(j=0;j<256;j++)
            index_block_current.index[j]=-1;  //索引块初始化
        for(i=9;i<num_block;i++)
            index_block_current.index[i-9]=block_no[i];
        //索引块写入size磁盘
        fseek(disk,index_block_no*512,0);
        fwrite(&index_block_current,512,1,disk);
    }
    fclose(disk);
    return 0;
}
int mkdir(char *dirname,short userid)
{    //在当前目录下新建一个目录
    //注意新建目录时对目录项的初始化（置空）
    //注意检测目录重名
    int i;
    int pos_user=online(userid);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    short new_inode_no=new_inode();
    //在目录栈栈顶通过i结点号访问磁盘找到当前目录文件，在目录项表中填入分配的i结点号和目录名
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //从各个索引位置读取当前目录文件的数据
    fseek(disk,current_dir_dinode.i_addr[0]*512,0);
    fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[1]*512,0);
    fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[2]*512,0);
    fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[3]*512,0);
    fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[4]*512,0);
    fread(&(curret_dir.size),4,1,disk);  //128/4


  /*  fseek(disk,offset_fatherdir_data,0);   //------------------测试点，因为根目录文件连续，暂不考虑物理索引的离散性
    fread(&curret_dir,2052,1,disk);*/
    if(curret_dir.size>(DIRNUM-1))
    {
        printf("当前目录目录项数达到上限！！！\n");
        return 0;
    }
    //检测目录重名(目录名和文件名也不能重复
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,dirname)==0)
        {
            printf("1.目录或文件重名！！！操作失败！！！\n");
            return 0;
        }

    }
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no==-1)  //找到空目录项
        {
            curret_dir.direct_list[i].inode_no=new_inode_no;
            strcpy(curret_dir.direct_list[i].file_name,dirname);
            break;
        }
    }
    curret_dir.size++;   //当前目录的项数加1
    //从各个索引位置读取当前目录文件的数据
    fseek(disk,current_dir_dinode.i_addr[0]*512,0);
    fwrite(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[1]*512,0);
    fwrite(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[2]*512,0);
    fwrite(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[3]*512,0);
    fwrite(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[4]*512,0);
    fwrite(&(curret_dir.size),4,1,disk);  //128/4
   /* fseek(disk,offset_fatherdir_data,0);
    fwrite(&curret_dir,2052,1,disk);   //将修改后的当前目录文件写入数据区*/
    //从磁盘取到分配的i结点，设置文件类型为目录文件,以及i结点的其他表项
    struct dinode new_created_dinode;                //---------------------------监视位置
    int offset_new_dinode=1024+new_inode_no*32;
    new_created_dinode.inode_no=new_inode_no;
    new_created_dinode.inode_type=0;
    new_created_dinode.i_mode=1;   //目录文件的读写权限默认为低级
    new_created_dinode.file_size=5;  //目录文件大小为5
    giveblock(new_created_dinode.i_addr,5);  //分配块   -------------------------错误点1，分配块的时候其他索引写为空
    fseek(disk,offset_new_dinode,0);
    fwrite(&new_created_dinode,28,1,disk);  //写入新目录的i结点
    //将空目录文件写入new_created_dinode.i_addr对应的物理地址
    struct dir empty_dir;
    empty_dir.size=0;
    for(i=0;i<DIRNUM;i++)
    {
        empty_dir.direct_list[i].inode_no=-1;
        strcpy(empty_dir.direct_list[i].file_name,"");
    }
    fseek(disk,new_created_dinode.i_addr[0]*512,0);
    fwrite(&(empty_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,new_created_dinode.i_addr[1]*512,0);
    fwrite(&(empty_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,new_created_dinode.i_addr[2]*512,0);
    fwrite(&(empty_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,new_created_dinode.i_addr[3]*512,0);
    fwrite(&(empty_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,new_created_dinode.i_addr[4]*512,0);
    fwrite(&(empty_dir.size),4,1,disk);  //128/4


    //询问是否进入新建目录
    printf("是否进入新建目录(y/n)");
    char x[5];
    fclose(disk);
    while(1)
    {
        scanf("%s",x);
        if(strcmp(x,"y")==0)
        {
            open(dirname,userid);
            break;
        }
        else if(strcmp(x,"n")==0)
        {

            break;   //不需要修改当前目录
        }
        else
        {

            printf("重新输入：\n");
        }
    }
getchar();
    return 0;
}


short new_inode()
{    //这里只负责给出新的i结点号并修改超级块（内存中的），新建结点的初始化将在主调函数中进行。
    int i;
    if(internal_filsys.freeinode_stack_num>0)
    {
        //internal_filsys.inode_num--;
        internal_filsys.changed='y';
        int x=(--internal_filsys.freeinode_stack_num);
        return internal_filsys.freeinode_stack[x];
    }
    else    //空闲i结点栈空
    {
        int have_freeinode_flag=0;
        struct dinode temp_dinode;

        for(i=internal_filsys.flag_inode;i<512;i++)   //从铭记i结点开始找空闲i结点
        {
            iget(i,&temp_dinode);
            if(temp_dinode.inode_type==-1)
            {
                have_freeinode_flag=1;
                internal_filsys.freeinode_stack[internal_filsys.freeinode_stack_num++]=temp_dinode.inode_no;  //入栈
                if(internal_filsys.freeinode_stack_num==50)   //栈满
                {

                    int y=(--internal_filsys.freeinode_stack_num);
                    return internal_filsys.freeinode_stack[y];
                }
            }
        }
        if(have_freeinode_flag==0&&i==512)
        {
            //internal_filsys.changed='n';   //只标记修改，不标记不修改
            printf("没有空闲i结点，请关闭一些文件再打开。\n");
            return 0;
        }
        //剩余i结点不足50个的情况
        int z=(--internal_filsys.freeinode_stack_num);
        internal_filsys.changed='y';
        return internal_filsys.freeinode_stack[z];
    }
    return 0;
}

int get_new_inode()   //新建i结点测试函数
{
    int x;
    x=new_inode();
    struct dinode y;
    y.inode_type=0;
    y.inode_no=x;
    iput(x,&y);
    return 0;
}
int release_dinode(short dinode_no)
{     //释放磁盘i结点(需要将i结点类型,i结点号改为空写回）
    struct dinode empty_dinode;
    empty_dinode.inode_type=-1;
    empty_dinode.inode_no=dinode_no;
    if(dinode_no<internal_filsys.flag_inode)   //要释放的i结点号小于铭记i结点
    {
        if(internal_filsys.freeinode_stack_num==50)   //栈满，仅把空闲i结点写回磁盘不修改空闲i结点栈
            {
                internal_filsys.flag_inode=dinode_no;   //修改铭记i结点
                internal_filsys.changed='y';
                iput(dinode_no,&empty_dinode);   //写回空闲i结点
            }
        else
            {
                internal_filsys.changed='y';
                iput(dinode_no,&empty_dinode);   //写回空闲i结点
                internal_filsys.freeinode_stack[internal_filsys.freeinode_stack_num++]=dinode_no;  //入栈
            }
    }
    else
    {
        if(internal_filsys.freeinode_stack_num!=50)   //栈不满
            {
                internal_filsys.changed='y';
                iput(dinode_no,&empty_dinode);   //写回空闲i结点
                internal_filsys.freeinode_stack[internal_filsys.freeinode_stack_num++]=dinode_no;  //入栈
            }
        else  //栈满，仅写回空闲i结点
            iput(dinode_no,&empty_dinode);   //写回空闲i结点;
    }
    return 0;
}
int read_superblock()            //第一次写完时发生读错了，格式化后对了
{
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    fseek(disk,512,0);
    fread(&internal_filsys,315,1,disk);
    internal_filsys.changed='n';
    return 0;
}
int write_superblock()  //写回超级块
{
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    fseek(disk,512,0);
    fwrite(&internal_filsys,315,1,disk);
    fclose(disk);
    printf("写回超级块\n");
    return 0;
}
int show_empty_inode()  //显示空闲i结点栈内容
{
    int i;
    for(i=0;i<internal_filsys.freeinode_stack_num;i++)
        printf("%d ",internal_filsys.freeinode_stack[i]);
    printf("\n");
    return 0;
}
int quit()
{
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    int i,j;
    //写回超级块
    printf("2.internal_filsys.changed=%c\n",internal_filsys.changed);
    if(internal_filsys.changed=='y')
        write_superblock();//写回超级块

    //内存i结点写回磁盘i结点
    for(i=0;i<NHINO;i++)
    {
        struct dinode write_back_dinode;
        struct hinode current_hinode;   //用于在哈希链表中遍历
        current_hinode.i_forw=hinode_index[i].i_forw;
        if(hinode_index[i].i_forw!=NULL)
        {
            while(1)
            {
                if(current_hinode.i_forw->change_flag=='y')
                {
                    //写回i结点

                    write_back_dinode.inode_no=current_hinode.i_forw->inode_no;
                    write_back_dinode.file_size=current_hinode.i_forw->file_size;
                    write_back_dinode.inode_type=current_hinode.i_forw->inode_type;
                    for(j=0;j<10;j++)
                        write_back_dinode.i_addr[j]=current_hinode.i_forw->i_addr[j];
                    write_back_dinode.i_mode=current_hinode.i_forw->i_mode;
                    fseek(disk,write_back_dinode.inode_no*32+1024,0);
                    fwrite(&write_back_dinode,28,1,disk);
                }
                current_hinode.i_forw=current_hinode.i_forw->next;
                if(current_hinode.i_forw==NULL)
                    break;
            }

        }
    }
    fclose(disk);
    return 0;
}

int get_dirdetail(int currentdir_inode_no)   //获得当前目录的详细信息     -----bug_key3
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    struct dinode current_dir_dinode;
    iget(currentdir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //从各个索引位置读取当前目录文件的数据
    fseek(disk,current_dir_dinode.i_addr[0]*512,0);
    fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[1]*512,0);
    fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[2]*512,0);
    fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[3]*512,0);
    fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[4]*512,0);
    fread(&(curret_dir.size),4,1,disk);  //128/4

    struct dinode temp;
    for(i=0;i<DIRNUM;i++)
    {   //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>LLLLLLLLLLLLLLLLL
        //写当前目录详细信息为空
        current_dirdetail[i].inode_no=-1;   //对上一次查找结果的清空
        if(curret_dir.direct_list[i].inode_no!=-1)
        {
            current_dirdetail[i].inode_no=curret_dir.direct_list[i].inode_no;
            strcpy(current_dirdetail[i].filename,curret_dir.direct_list[i].file_name);
            //由目录项的i结点号找到文件类型
            fseek(disk,1024+32*current_dirdetail[i].inode_no,0);
            fread(&temp,28,1,disk);    //--------------------------改为fread
            current_dirdetail[i].file_type=temp.inode_type;
        }
    }


    fclose(disk);
    return 0;
}
int show_dir(short userid)
{
    int i,j=1;
    int dir_inode_no;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==userid)
    {
        dir_inode_no=user_file_list[i].route_stack[user_file_list[i].route_stack_head-1];
        break;
    }
    if(i==USERNUM)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    //printf("inode_no_fatherdir=%d\n",dir_inode_no);
    printf("\n");
    get_dirdetail(dir_inode_no);
    showroute(userid);  //输出当前路径
    printf("-------------------------------------------------------------\n");
    printf("序号          文件名         i结点号                 文件类型|\n");
    for(i=0;i<DIRNUM;i++)
        if(current_dirdetail[i].inode_no!=-1)
        {
            if(current_dirdetail[i].file_type==1)
             {
                 printf("%d             %s             %d                    ",j++,current_dirdetail[i].filename,current_dirdetail[i].inode_no);
                 printf("文本文件|\n");
             }
            else
            {
                printf("%d             %s             %d                    ",j++,current_dirdetail[i].filename,current_dirdetail[i].inode_no);
                printf("  目录|\n");
            }
        }
    printf("-------------------------------------------------------------\n");
    return 0;
}
int back_dir(short userid)
{
    int pos_user=online(userid);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    if(user_file_list[pos_user].route_stack_head==1)
    {
        printf("禁止退出根目录!1!\n");
        return 0;
    }
    user_file_list[pos_user].route_stack_head--;
    return 0;
}
int create(char *filename,short userid)
{
    int i;
    int pos_user=online(userid);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    short new_inode_no=new_inode();
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //从各个索引位置读取当前目录文件的数据
    fseek(disk,current_dir_dinode.i_addr[0]*512,0);
    fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[1]*512,0);
    fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[2]*512,0);
    fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[3]*512,0);
    fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[4]*512,0);
    fread(&(curret_dir.size),4,1,disk);  //128/4



    if(curret_dir.size>(DIRNUM-1))
    {
        printf("当前目录目录项数达到上限！！！\n");
        return 0;
    }
    //检测目录重名(目录名和文件名也不能重复
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,filename)==0)
        {
            printf("2.目录或文件重名！！！操作失败！！！\n");
            return 0;
        }

    }
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no==-1)  //找到空目录项
        {
            curret_dir.direct_list[i].inode_no=new_inode_no;
            strcpy(curret_dir.direct_list[i].file_name,filename);
            break;
        }
    }
    curret_dir.size++;   //当前目录的项数加1
    //将修改后的目录文件写回磁盘
    fseek(disk,current_dir_dinode.i_addr[0]*512,0);
    fwrite(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[1]*512,0);
    fwrite(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[2]*512,0);
    fwrite(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[3]*512,0);
    fwrite(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[4]*512,0);
    fwrite(&(curret_dir.size),4,1,disk);  //128/4

    //i结点的分配和写入
    struct dinode new_created_dinode;                //---------------------------监视位置
    int offset_new_dinode=1024+new_inode_no*32;
    new_created_dinode.inode_no=new_inode_no;
    new_created_dinode.inode_type=1;
    new_created_dinode.i_mode=1;   //文本文件的读写权限默认为低级
    new_created_dinode.file_size=0;  //空文件
    giveblock(new_created_dinode.i_addr,0);  //分配块   -------------------------错误点1，分配块的时候其他索引写为空
    fseek(disk,offset_new_dinode,0);
    fwrite(&new_created_dinode,28,1,disk);  //写入新目录的i结点

    fclose(disk);
    return 0;
}
int upfile(char *filename,short userid)   //管理员取得当前目录下某个文件的所有权
{
    int i;
    int pos_user=online(userid);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    if(user_file_list[pos_user].group_id==1)
    {
        printf("禁止非管理员获得管理员所有权！！！\n");
        return 0;
    }
    int file_inode_no;  //要升级的文件的
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //从各个索引位置读取当前目录文件的数据
    fseek(disk,current_dir_dinode.i_addr[0]*512,0);
    fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[1]*512,0);
    fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[2]*512,0);
    fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[3]*512,0);
    fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[4]*512,0);
    fread(&(curret_dir.size),4,1,disk);  //128/4


    //检测文件是否存在
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,filename)==0)
        {
            file_inode_no=curret_dir.direct_list[i].inode_no;   //获得要升级文件的i结点号
            break;
        }

    }
    if(i==DIRNUM)
    {
        printf("文件不存在！！！\n");
        return 0;
    }
    struct dinode new_dinode;
    fseek(disk,1024+32*file_inode_no,0);
    fread(&new_dinode,28,1,disk);
    new_dinode.i_mode=0;
    fseek(disk,1024+32*new_dinode.inode_no,0);
    fwrite(&new_dinode,28,1,disk);   //升级后的i结点写回磁盘
    fclose(disk);
    return 0;
}

int downfile(char *filename,short userid)   //管理员取消当前目录下某个文件的所有权
{
    int i;
    int pos_user=online(userid);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    if(user_file_list[pos_user].group_id==1)
    {
        printf("禁止非管理员获得管理员所有权！！！\n");
        return 0;
    }
    int file_inode_no;  //要升级的文件的
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //从各个索引位置读取当前目录文件的数据
    fseek(disk,current_dir_dinode.i_addr[0]*512,0);
    fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[1]*512,0);
    fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[2]*512,0);
    fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[3]*512,0);
    fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

    fseek(disk,current_dir_dinode.i_addr[4]*512,0);
    fread(&(curret_dir.size),4,1,disk);  //128/4


    //检测文件是否存在
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,filename)==0)
        {
            file_inode_no=curret_dir.direct_list[i].inode_no;   //获得要升级文件的i结点号
            break;
        }

    }
    if(i==DIRNUM)
    {
        printf("文件不存在！！！\n");
        return 0;
    }
    struct dinode new_dinode;
    fseek(disk,1024+32*file_inode_no,0);
    fread(&new_dinode,28,1,disk);
    new_dinode.i_mode=1;
    printf("id=%d level=%d\n",new_dinode.inode_no,new_dinode.i_mode);
    fseek(disk,1024+32*new_dinode.inode_no,0);
    fwrite(&new_dinode,28,1,disk);   //升级后的i结点写回磁盘
    fclose(disk);
    return 0;
}
int close(char *filename,short userid)
{
    int i;
    int pos_user=online(userid);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    int num_samename=0;

    for(i=0;i<NOFILE;i++)   //用户
    {//user_file_list[USERNUM]
        int index_sys=user_file_list[pos_user].u_ofile[i];   //用户打开文件在系统表中的索引位置
        if(strcmp(sys_ofile[index_sys].filename_open,filename)==0)
        {
            if(sys_ofile[index_sys].inode_no!=-1)   //双保险，确认已经打开
            {
                int file_inode_no=sys_ofile[i].inode_no;   //获得文件i结点(删除时释放i结点物理块有用）
                //修改系统打开表

                if(sys_ofile[index_sys].read_count+sys_ofile[index_sys].write_count==1)  //只有一个用户打开
                {

                    strcpy(sys_ofile[index_sys].filename_open,"");
                    sys_ofile[index_sys].read_count=0;
                    sys_ofile[index_sys].read_count=0;
                    sys_ofile[index_sys].inode_no=-1;

                    //释放内存i结点
                    if(hinode_index[file_inode_no%128].i_forw==sys_ofile[index_sys].f_inode)  //哈希链表对应索引位置第一个i结点为当前内存i结点
                        hinode_index[file_inode_no%128].i_forw=NULL;
                    else
                    {
                        sys_ofile[index_sys].f_inode->last->next=sys_ofile[index_sys].f_inode->next;
                        if(sys_ofile[index_sys].f_inode->next!=NULL)
                            sys_ofile[index_sys].f_inode->next->last=sys_ofile[index_sys].f_inode->last;
                    }
                }
                else   //有多个用户进程在读
                    sys_ofile[index_sys].read_count--;






                //修改用户表
                user_file_list[pos_user].u_ofile[i]=-1;
            }
            num_samename++;
        }
    }
    if(num_samename<2)
        printf("关闭成功。\n");
    else
        printf("关闭%d个同名文件\n",num_samename);
    return 0;
}

int deletefile(char *filename,short userid)
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    int pos_user=online(userid);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    //当前目录下搜索该文件，判断是否存在；获取i结点号;获取文件类型
    int current_dir_inode_no;//当前目录i结点
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];   //当前用户路径栈栈栈顶为当前目录
    get_dirdetail(current_dir_inode_no);   //获取该用户当前目录详细信息//----bug_key2
    int file_type;    //要打开文件的类型
    int file_inode_no;   //要删除文件的i结点号
    for(i=0;i<DIRNUM;i++)
    {    //根据文件名在当前目录中寻找
        if(current_dirdetail[i].inode_no!=-1&&(strcmp(current_dirdetail[i].filename,filename)==0))
            {
                file_type=current_dirdetail[i].file_type;    //--------------   x=y  -1
                file_inode_no=current_dirdetail[i].inode_no;
                break;
            }
    }
    if(i==DIRNUM)//没有找到
    {
        printf("文件不存在!!!\n");
        return 0;
    }
    //判断文件类型
    if(file_type==1)  //删除文本文件
    {
        //判断是否被系统打开
        int sys_file_open_flag=0;  //系统打开文件表是否已经打开文件的标记

        for(i=0;i<SYSOPENFILE;i++)   //判断系统打开文件表中有没有目录文件的打开结点
        {
            if(sys_ofile[i].inode_no==file_inode_no)
            {   //系统已打开该目录（其他用户进程）
                sys_file_open_flag=1;
                break;
            }
        }
        if(sys_file_open_flag==1)
        {
            printf("文件已经被打开，禁止删除!!!\n");
            return 0;
        }
        //开始删除
        //1.读取i结点
        FILE *disk;
        disk=fopen("disk","rb+");//文件指针指向磁盘文件
        fseek(disk,file_inode_no*32+1024,0);
        struct dinode file_dinode;
        fread(&file_dinode,28,1,disk);
        release_block(file_dinode.i_addr);
        release_dinode(file_inode_no);

        //删除目录文件中对应目录项
        struct dinode current_dir_dinode;
       // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
        //fread(&current_dir_dinode,28,1,disk);

        iget(current_dir_inode_no,&current_dir_dinode);

        struct dir curret_dir;
        //从各个索引位置读取当前目录文件的数据
        fseek(disk,current_dir_dinode.i_addr[0]*512,0);
        fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[1]*512,0);
        fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[2]*512,0);
        fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[3]*512,0);
        fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[4]*512,0);
        fread(&(curret_dir.size),4,1,disk);  //128/4

        //修改目录项


        for(i=0;i<DIRNUM;i++)
            if(curret_dir.direct_list[i].inode_no==file_inode_no)
        {
            curret_dir.direct_list[i].inode_no=-1;
            strcpy(curret_dir.direct_list[i].file_name,"");
            break;
        }
        curret_dir.size--;   //当前目录的项数加1
        //从各个索引位置读取当前目录文件的数据
        fseek(disk,current_dir_dinode.i_addr[0]*512,0);
        fwrite(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[1]*512,0);
        fwrite(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[2]*512,0);
        fwrite(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[3]*512,0);
        fwrite(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[4]*512,0);
        fwrite(&(curret_dir.size),4,1,disk);  //128/4
    }
    else  //删除目录文件
    {
        //要删除目录的i结点 = file_inode_no
        //进入该目录
        user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head++]=file_inode_no;
        //挨个删除每一个目录项对应的文件
        //1。读取目录文件
        struct dinode current_dir_dinode;
        int deep_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];
        //读取进入目录的目录文件
        iget(deep_dir_inode_no,&current_dir_dinode);

        struct dir curret_dir;
        //从各个索引位置读取当前目录文件的数据
        fseek(disk,current_dir_dinode.i_addr[0]*512,0);
        fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[1]*512,0);
        fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[2]*512,0);
        fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[3]*512,0);
        fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[4]*512,0);
        fread(&(curret_dir.size),4,1,disk);  //128/4

        for(i=0;i<DIRNUM;i++)
            if(curret_dir.direct_list[i].inode_no!=-1)
            deletefile(curret_dir.direct_list[i].file_name,userid);
        //退出该目录
        user_file_list[pos_user].route_stack_head--;
        //修改和写回目录文件
        int current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];
        //读取初始目录的目录文件
        iget(current_dir_inode_no,&current_dir_dinode);


        //从各个索引位置读取当前目录文件的数据
        fseek(disk,current_dir_dinode.i_addr[0]*512,0);
        fread(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[1]*512,0);
        fread(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[2]*512,0);
        fread(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[3]*512,0);
        fread(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[4]*512,0);
        fread(&(curret_dir.size),4,1,disk);  //128/4

        for(i=0;i<DIRNUM;i++)
            if(curret_dir.direct_list[i].inode_no==file_inode_no)
        {
            curret_dir.direct_list[i].inode_no=-1;
            strcpy(curret_dir.direct_list[i].file_name,"");
            break;
        }
        curret_dir.size--;   //当前目录的项数加1
        //从各个索引位置读取当前目录文件的数据
        fseek(disk,current_dir_dinode.i_addr[0]*512,0);
        fwrite(&(curret_dir.direct_list[0]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[1]*512,0);
        fwrite(&(curret_dir.direct_list[32]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[2]*512,0);
        fwrite(&(curret_dir.direct_list[64]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[3]*512,0);
        fwrite(&(curret_dir.direct_list[96]),512,1,disk);  //128/4

        fseek(disk,current_dir_dinode.i_addr[4]*512,0);
        fwrite(&(curret_dir.size),4,1,disk);  //128/4
    }
    fclose(disk);
    return 0;
}

int upuser(short userid_uped,short userid_up)  //提升普通用户为管理员用户
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    int pos_user=online(userid_up);   //在用户打开表表中的位置
    if(pos_user==-1)
    {
        printf("用户进程未登录！\n");
        return 0;
    }
    if(user_file_list[pos_user].group_id==1)
    {
        printf("禁止非管理员执行授权操作！！！\n");
        return 0;
    }
    //授权
    int pos_usered=online(userid_uped);   //在用户打开表表中的位置
    //1.内存用户表修改
    if(pos_usered!=-1)   //用户登录了
        user_file_list[pos_usered].group_id=0;
    //2.磁盘用户表修改
    struct user_process user_list[PWDNUM];  //磁盘静态用户表读取    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);
    for(i=0;i<PWDNUM;i++)
        if(user_list[i].user_id==userid_uped)
        {
            user_list[i].group_id=0;
            fseek(disk,39*512,0);
            fwrite(&user_list,512,1,disk);
            break;
        }
    fclose(disk);
    return 0;
}


int main()
{
    FILE *disk;
    disk=fopen("disk","rb+");//文件指针指向磁盘文件
    int i,j;
    printf("\n        --------------------欢迎进入UNIX VFS 系统--------------------\n\n\n");
    printf("是否要格式化？（y/n):");
    char x;
    while(1)
    {
        scanf("%c",&x);
        if(x=='y')
            {
                getchar();
                initialise();
                break;
            }
        else if(x=='n')
         {
             getchar();
             break;
         }
        else
        {
            getchar();
            printf("输入错误");
        }
    }
    read_superblock();
    user_file_list_initialise();  //用户打开文件表表表的初始化
    sys_ofile_initialise() ;     //系统打开文件表初始化
    hinode_index_initialise();   //内存i结点哈希链表索引表的初始化
    char order[50];
    char order_cutup[10][50];  //命令解析结果，第一个字符串为操作名
    int order_cutup_i;  //命令拆分成的字符串个数
    int len;  //命令长度
    while(1)
    {
        j=0;
        order_cutup_i=1;   //命令拆分成的字符串个数
        printf("输入命令：");
        gets(order);
        len=strlen(order);
        for(i=0;i<len;i++)
        {
            if(order[i]!=' ')
                order_cutup[order_cutup_i-1][j++]=order[i];
            else
            {
                order_cutup[order_cutup_i-1][j]='\0';
                j=0;
                order_cutup_i++;
            }
        }
        order_cutup[order_cutup_i-1][j]='\0';//最后一个参数的尾部（或者无参数命令尾部）
        if(strcmp(order_cutup[0],"login")==0)
            {
                if(order_cutup_i!=3)   //参数不足
                    {
                        printf("命令参数错误\n");
                        continue;
                    }
                login(order_cutup[1],order_cutup[2]);
            }
        else if(strcmp(order_cutup[0],"logout")==0)
            logout(short(atoi(order_cutup[1])));
        else if(strcmp(order_cutup[0],"showalluser")==0)
            showalluser();    //显示磁盘里的所有用户
        else if(strcmp(order_cutup[0],"showonlineuser")==0)
            showonlineuser();    //显示在线用户
        else if(strcmp(order_cutup[0],"deleteuser")==0)
            deleteuser(short(atoi(order_cutup[1])),short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"adduser")==0)
            adduser(short(atoi(order_cutup[1])),short(atoi(order_cutup[2])),order_cutup[3],short(atoi(order_cutup[4])));
        else if(strcmp(order_cutup[0],"quit")==0)
            {
                quit();
                break;
            }
        else if(strcmp(order_cutup[0],"open")==0)
            open(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"delete")==0)
            deletefile(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"showroute")==0)
            showroute(short(atoi(order_cutup[1])));
        else if(strcmp(order_cutup[0],"mkdir")==0)   //新建目录文件
            mkdir(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"create")==0)   //新建目录文件
            create(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"upfile")==0)   //新建目录文件
            upfile(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"downfile")==0)   //新建目录文件
            downfile(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"upuser")==0)   //新建目录文件
            upuser(short(atoi(order_cutup[1])),short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"close")==0)   //新建目录文件
            close(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"showemptyi")==0)
            show_empty_inode();
        else if(strcmp(order_cutup[0],"releasei")==0)
            release_dinode(short(atoi(order_cutup[1])));
        else if(strcmp(order_cutup[0],"newi")==0)
            get_new_inode();
        else if(strcmp(order_cutup[0],"cls")==0)
            system("cls");
        else if(strcmp(order_cutup[0],"dir")==0)
            show_dir(short(atoi(order_cutup[1])));
        else if((strcmp(order_cutup[0],"cd")==0)&&(strcmp(order_cutup[1],"..")==0))
            back_dir(short(atoi(order_cutup[2])));

        else
        {
            printf("输入命令有误！！！\n");
        }

    }
    fclose(disk);
    return 0;
}
