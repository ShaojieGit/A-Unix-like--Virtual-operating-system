//���̳�ʼ��ģ��2.0���޸ģ������û����̱�������������(����i������i�������
//�û���¼
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#define DIRNUM      128     //ÿ��Ŀ¼�������������
#define NHINO       128         //�ڴ�i����ϣ����Ĵ�С��ȡ������)
#define NOFILE      20      //�û����̴򿪱���������
#define SYSOPENFILE 40    //���������ļ�����ϵͳ���ļ���������
#define USERNUM     10      //���10���û�(�����ļ�������������
#define PWDNUM      32      //ϵͳ����¼�û���
#define ROUTENUM    40      //���·�����
struct dinode{ //����I��㣨ÿ��32���ֽ�,����ռ������28���ֽ�)
    short inode_no;   //i���ţ��ڲ���ʶ����(-1Ϊ�գ�
    short inode_type;  //i������ͣ�-1Ϊ�ս�㣬0ΪĿ¼�ļ���1Ϊ�ı��ļ���
    short i_mode;     //��дȨ��  (0Ϊ�߼�������Ա������Ȩ��1Ϊ�ͼ��������Զ�д��
    short file_size;    //�ı����ֽ�����Ŀ¼�ļ�д��5�ˣ�
    short i_addr[10];    //�����ַ������
};  //����Ҫһ�������ֶα���ǲ��ǿյ�i���

struct inode{    //�ڴ�i���NICFREE
    struct inode *last;
    struct inode *next;
    //int user_sharenum;   //�������(���û�������     ��ϵͳ�򿪱��н���ͳ�ƹ����������д���������û����
    //int disk_sharenum;    //���̹��������д��ݷ�ʽʱʹ��
    char change_flag;    //�޸ı�־    (0Ϊδ�޸ģ�1Ϊ�޸ģ�

    //----����Ϊ�Ӵ���i��㸴�ƹ����Ĳ���
    short inode_no;   //i���ţ��ڲ���ʶ����
    short inode_type;  //i������ͣ�-1Ϊ�ս�㣬0ΪĿ¼�ļ���1Ϊ�����ļ���2Ϊ��ͨ�ļ���
    int i_mode;     //��дȨ��   ��0Ϊ�߼���1Ϊ�ͼ���
    int file_size;    //�ļ���С�����ٿ飩
    int i_addr[10];    //�����ַ������
};

struct direct{  //Ŀ¼���ļ���ÿ��16B)
    short inode_no;   //i���ţ��ڴ�i���)(2B)
    char file_name[14];   //�ļ���(14B)
};

struct filsys{   //�����죨����죩���ܷ���һ����������飩(315���ֽڣ�
    //int inode_num;   //ʣ�����i������
    //int disk_size;     //������̿���   ????��ë��
    int freeblock_num;  //ʣ����п����
    int freeblock_stack_num;  //���п�ջ�п��п�����������ϵͳ�̳���ʵ��p230�鳤���һ�������Ƶ�����
    int freeblock_stack[50];   //���п�ջ(�������㷨��ÿ�����Ϊ50
    int freeinode_stack_num;    //����i���ջ�п���i�������
    short freeinode_stack[50];   //����i���ջ (ջ��СΪ50�����i�������㷨�йأ��ο��ҵ��Ŀ⡶�����ѧ����ϵͳ�̳�P10��
    short flag_inode;  //����i��㣬�������һ�����i���ջ�����ã��ڴ���i���������������һ��i��㣬��������ջ��ʱ��������i������ʼλ�ã��ο���һ�����ϣ�(����Ϊ����i��㲻�ڵ�ǰ����i���ջ�У������´���������i���ʱ��Ϊ��һ�����п飡����
    char changed;  //���������ڴ��Ƿ��޸ĵı�ʶ
};

struct user_process{   //�û�����
    short user_id;  //�û����̱��
    short group_id;  //�û����   0��Ϊϵͳ���̣�����Ա����1��Ϊ��ͨ�û�����
    char password[12];  //����
};


struct dir{   //Ŀ¼�ļ�   2048+4=2052B��5�����ݿ�

    struct direct direct_list[DIRNUM];   //DIRNUM=128,ÿ��Ŀ¼�������������   16*128=2048B=4�����ݿ�
    int size;  //Ŀ¼�ļ�������
};


struct hinode{   //��ϣ�������
		struct inode *i_forw;
};
struct hinode hinode_index[NHINO];   //�ڴ�i����ϣ����������,NHINO=128, �ڴ�i����ϣ����Ĵ�С��ȡ������)

struct system_file{  //ϵͳ���ļ������    ������ϵͳ�̳���ʵ�顷226
    char open_type;   //�򿪷�ʽ��������д��   0Ϊд��1Ϊ��
    int read_count;  //�����̸���
    int write_count;   //д���̸���route_dinode
    short inode_no;     //i���ţ����������ڴ�i����ϣ��������λ��
    char filename_open[14];   //ϵͳ�򿪵��ļ���
    struct inode *f_inode; //���ļ��ڴ�i���ָ��,ͨ�����ڴ�i����ϣ�������������
    //int f_off;  //�ļ���дָ�룿����������������
};
struct system_file sys_ofile[SYSOPENFILE];   //ϵͳ���ļ���SYSOPENFILE=40���������ļ�����ϵͳ���ļ���������

struct user_file{   //�û����ļ������
    //int default_mode;   //Ĭ�ϴ򿪷�ʽ������������???????????
    int u_ofile[NOFILE];   //ϵͳ���ļ����������NOFILE=20
    short user_id;  //�û����̱��
    short group_id;   //�û����
    short route_stack[ROUTENUM];   //·��ջ,���Ŀ¼�ļ���i���ţ�ջ��Ϊ��ǰĿ¼��   ���40��
    char dirname_stack[ROUTENUM][14];    //·����ջ����������ʾ��ǰ·��
    int route_stack_head;   //·��ջջ��(��������ջ����һ��ջָ��)
};

struct index_block{   //����������
    short index[256];
};
struct dirdetail
{
    short inode_no;
    char filename[14];
    short file_type;
};
struct dirdetail current_dirdetail[DIRNUM];
struct user_file user_file_list[USERNUM];  //��ǰ�ѵ�¼�û�(���10����
struct filsys internal_filsys;   //�ڴ泬���飬ÿ������ʱ��д
struct block_grouphead{  //�鳤��
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
int user_file_list_initialise();  //�û����ļ�����ĳ�ʼ��
int sys_ofile_initialise() ;     //ϵͳ���ļ����ʼ��
int hinode_index_initialise();   //�ڴ�i����ϣ����������ĳ�ʼ��

int initialise()   //��ʼ��
{
    //�����������ڳ����й������̣���֮����Ϊһ���������ļ���������
    int i;

    /*���ݽṹʵ��*/
    FILE *disk;    //����ģ���ļ�
    struct filsys myfilsys;



    /*������Ŵ��̵��ļ�*/
    disk=fopen("disk","wb+");
    fseek(disk,279552,0);  //����Ϊ(1+1+32+512)*512B
    fclose(disk);
    disk=NULL;
    disk=fopen("disk","rb+");//���´�
    /*������ĳ�ʼ��*/
    //myfilsys.inode_num=511;   //��ʼʱʣ��511������i��㣬��Ϊ��һ����Ŀ¼��i���
    //myfilsys.disk_size=512-5-1;
    myfilsys.freeblock_num=506;  //512-5-1����ȥ��Ŀ¼�ļ�ռ��5���飬�û���1����
    myfilsys.freeblock_stack_num=7;
    for(i=0;i<7;i++)   //���ݳ����������㷨�������ʼʱ���һ����п�Ϊ8��
        myfilsys.freeblock_stack[i]=40+i;   //��34-38����Ϊ��Ŀ¼�ļ���,��39��Ϊ�û������п�ӵ�40�鿪ʼ
    for(i=7;i<50;i++)   //ջ��Ŀ�����
        myfilsys.freeblock_stack[i]=-1;
    myfilsys.freeinode_stack_num=50;
    for(i=0;i<50;i++)
        myfilsys.freeinode_stack[i]=i+1;  //��ʼ��ʱ���һ��i���Ϊ��Ŀ¼�ļ���i���
    myfilsys.flag_inode=51;    //����i���
    myfilsys.changed='n';
    /*filsysд����̳�����*/
    fseek(disk,512,0);
    fwrite(&myfilsys,315,1,disk);
    /*fseek(disk,512,0);    //���Գ������Ƿ�д��ɹ�
    struct filsys filsys_receive;
    fread(&filsys_receive,425,1,disk);
    printf(">>>>>%d",filsys_receive.flag_inode);
    fclose(disk);*/

    /*��Ŀ¼�ļ�i���*/

    struct dinode rootdir_inode;
    rootdir_inode.inode_no=0;   //��Ŀ¼�ļ�i���Ϊ��0��i���
    rootdir_inode.inode_type=0;  //Ŀ¼�ļ�
    rootdir_inode.i_mode=1;  //��Ŀ¼Ȩ��Ϊ�ͼ�
    rootdir_inode.file_size=5;  //Ŀ¼�ļ���СΪ5��
    for(i=0;i<10;i++)
        rootdir_inode.i_addr[i]=-1;  //����Ϊ��ʱ����-1
    for(i=0;i<5;i++)    //��Ŀ¼�ļ�5�����ݿ������
        rootdir_inode.i_addr[i]=34+i;
    fseek(disk,1024,0);
    fwrite(&rootdir_inode,28,1,disk);

    /*i���ȥ������i���*/
    struct dinode rootdir_inode_empty;
    rootdir_inode_empty.inode_type=-1;
    int pointer_i;  //������i���ʱ��Ϊָ��
    for(i=1;i<512;i++)
    {
        rootdir_inode_empty.inode_no=i;
        pointer_i=1056+(i-1)*32;
        fseek(disk,pointer_i,0);
        fwrite(&rootdir_inode_empty,28,1,disk);
    }


 /*  fseek(disk,1056,0);    //����
    struct dinode rootdir_inode_receive;
    fread(&rootdir_inode_receive,28,1,disk);
    printf("\n>>>>>>>>>>>>>>>%d",rootdir_inode_receive.inode_type);
*/
    /*��Ŀ¼�ļ�*/
    /*struct dir{   //Ŀ¼�ļ�   2048+4=2052B��5�����ݿ�
    struct direct direct[DIRNUM];   //DIRNUM=128,ÿ��Ŀ¼�������������   16*128=2048B=4�����ݿ�
    int size;  //Ŀ¼�ļ�������
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
    int x;     //����1
    short y;
    fseek(disk,19456,0);
    fread(&x,4,1,disk);
    printf("%d",x);
    return 0;*/
    /*struct dir rootdir_receive;   //����2
    fseek(disk,17408,0);
    fread(&rootdir_receive,2052,1,disk);
    printf("%d",rootdir_receive.size);*/

    /*���ݿ��ʼ���������������㷨�ĳ�ʼ��*/
    int block_index[50];
    int j=0;
    block_index[j++]=-1;   //�����ڶ����鳤���һ������Ϊ��
    for(i=545;i>=497;i--)
    {
        block_index[j++]=i;
    }   //�ڶ����鳤�����
    //now,i=496
    /*struct block_grouphead{  //�鳤��    200B
    int block_index[50];
};*/
    struct block_grouphead second ;    //���ڴ洢�ڶ����鳤��Ľṹ��
    for(i=0;i<50;i++)
        second.block_index[i]=block_index[i];
    fseek(disk,253952,0);    //496*512=253952   ��496��Ϊ�ڶ����鳤��
    fwrite(&second,200,1,disk);

    struct block_grouphead head_block;

    head_block.block_index[0]=496;
    int temp=0;
    for(i=495;i>=96;i--)   //iΪ���һ���鳤����
    {
        temp++;
        if(temp==50)
        {
            //��head_block��Ϊ�鳤��д�뵱ǰ��
            fseek(disk,i*512,0);
            fwrite(&head_block,200,1,disk);
            //��ǰ�����Ϊ��һ���鳤��ĵ�һ����user_list_current_num��
            head_block.block_index[0]=i;
            temp=0;
        }
        else   //��ǰ��ż���head_block.block_index[]
        {
            head_block.block_index[temp]=i;
        }
    }
    /*���һ�飨�����һ������)*/
    for(i=0;i<50;i++)
        head_block.block_index[i]=96-i;
    fseek(disk,40*512,0);
    fwrite(&head_block,200,1,disk);

    /*fseek(disk,512*40,0);    //����
    struct block_grouphead head_block_receive;
    fread(&head_block_receive,200,1,disk);
    for(i=0;i<50;i++)
        printf("%d ",head_block_receive.block_index[i]);*/

    /*���̾�̬�û����ʼ��*/
    /*
    struct user_process{   //�û�����    16B
    short user_id;  //�û����̱��
    short group_id;  //�û����   0��Ϊϵͳ���̣�����Ա����1��Ϊ��ͨ�û�����
    char password[12];  //����
};*/
    struct user_process user_list[PWDNUM];  //���̾�̬�û���    16*32=512B
    user_list[0].user_id=0;   //0���û�
    user_list[0].group_id=0;   //����Ա
    strcpy(user_list[0].password,"123");
    for(i=1;i<32;i++)
        user_list[i].user_id=-1;   //����31���û�Ϊ��
    fseek(disk,39*512,0);
    fwrite(&user_list,512,1,disk);

    /*fseek(disk,39*512,0);    //����
    struct user_process user_list_receive[PWDNUM];  //���̾�̬�û���    16*32=512B
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
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
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
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    int inode_pointer=1024+32*dinodeid;
    fseek(disk,inode_pointer,0);
    fwrite(inode_write,28,1,disk);
    fclose(disk);
    disk=NULL;
    return 0;
}
int format()   //��ʽ��
{
    initialise();
    read_superblock();
    user_file_list_initialise();  //�û����ļ�����ĳ�ʼ��
    sys_ofile_initialise() ;     //ϵͳ���ļ����ʼ��
    hinode_index_initialise();   //�ڴ�i����ϣ����������ĳ�ʼ��
    return 0;
}

int user_file_list_initialise()   //��ǰ�ѵ�¼�û����̱�ĳ�ʼ��
{
    int i,j;
    for(i=0;i<USERNUM;i++)                         //��ʼ����ǰ�û����̱�
        {
            user_file_list[i].user_id=-1;
            for(j=0;j<NOFILE;j++)
                {
                    user_file_list[i].u_ofile[j]=-1;  //��ʼʱ�û�û�д��ļ�
                    user_file_list[i].route_stack_head=0; //��ʼʱ·��ջΪ��
                }
        }
    return 0;
}
int sys_ofile_initialise()     //ϵͳ���ļ���ĳ�ʼ��
{
    int i;
    for(i=0;i<SYSOPENFILE;i++)
    {
        sys_ofile[i].read_count=0;
        sys_ofile[i].write_count=0;
        sys_ofile[i].inode_no=-1;
        sys_ofile[i].f_inode=NULL;  //�ڴ�i����ÿ�
        strcpy(sys_ofile[i].filename_open,"");
    }
    return 0;
}
int hinode_index_initialise()   //�ڴ�i����ϣ����������ĳ�ʼ��
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
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�

    struct user_process user_list[PWDNUM];  //���̾�̬�û����ȡ    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);
    for(i=0;i<PWDNUM;i++)
        if(user_list[i].user_id==userid)
        {
            printf("�û�����ID��ͻ,���ʧ�ܣ�����\n");
            return 0;
        }

    short father_groupid=-1;
    //�жϸ����û��Ƿ��ѵ�¼
    int pos_father=online(father);
    if(pos_father==-1)
    {
        printf("������δ��¼��\n");
        return 0;
    }
    father_groupid=user_file_list[pos_father].group_id;
    if(father_groupid>group_id)   //��ͨ�û���������Ա�û����������i��ǰ���i�й���
    {
        printf("Ȩ�޲��㣡");
        return 0;
    }
    //-------------------------------�ж��Ƿ����ID��ͬ���û�����



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
        printf("������δ��¼��\n");
        return 0;
    }

    //�ж��½��û��ļ����Ƿ��븸�����Ǻ�
    if(father_groupid>group_id)   //��ͨ�û���������Ա�û����������i��ǰ���i�й���
    {
        printf("Ȩ�޲��㣡");
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
    //����洢λ��
    int pos;
    pos=39*512+16*i;
    fseek(disk,pos,0);
    fwrite(&user_tosave,16,1,disk);   //�洢���û�
    fclose(disk);
    return 0;
}
int login(char *param_userid,char *param_password)
{


    int i,j,k;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    short userid;
    //short groupid;
    char password[12];
    /*��ȡ����*/
    userid=short(atoi(param_userid));
    strcpy(password,param_password);

    struct user_process user_list[PWDNUM];  //���̾�̬�û����ȡ    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);

    //printf(">>>>>>>>>>>>>>>>%d  %s<<<<\n",user_list[1].user_id,user_list[1].group_id);  Ϊʲô�������ͱ���������
    int login_flag=0;   //�Ƿ��¼�ɹ�
    int repeat_login_flag=0;//�Ƿ��ظ���¼


        for(j=0;j<10;j++)
        {
            if(userid==user_file_list[j].user_id)
            {
                printf("�ظ���¼������\n");
                repeat_login_flag=1;
            }
        }
        if(repeat_login_flag==1)
            return 0;    //�ظ���¼���˳���¼�������´������¼����ʱ�ٵ�¼


        for(i=0;i<PWDNUM;i++)
            if(userid!=-1&&userid==user_list[i].user_id&&strcmp(password,user_list[i].password)==0)
            {   //������ȷ


                for(j=0;j<10;j++)
                    if(user_file_list[j].user_id==-1)
                    {
                        //�����û��洢����ǰ�û���
                        user_file_list[j].user_id=userid;
                        user_file_list[j].group_id=user_list[i].group_id;
                        user_file_list[j].route_stack_head=0;
                        for(k=0;k<NOFILE;k++)
                            user_file_list[j].u_ofile[k]=-1;  //�û���ϵͳ��������ʼ��
                        //user_file_list[j].route_stack_head=0;
                        //�򿪸�Ŀ¼-----------------------------------------------------------------------------
                        int pos_userid;  //�û����̱����û����̱���е�λ��
                        char open_file_name[14];
                        strcpy(open_file_name,"root");
                        pos_userid=online(userid);




                        //�û��ļ����޸�(��ʱ����ȷ�����Դ򿪸�Ŀ¼��
                        int stack_head=user_file_list[pos_userid].route_stack_head;
                        user_file_list[pos_userid].route_stack[stack_head]=0;   //��Ŀ¼i���ѹ��ջ��
                        strcpy(user_file_list[pos_userid].dirname_stack[stack_head],"root");   //��Ŀ¼��ѹ��·����ջ
                        user_file_list[pos_userid].route_stack_head++;
                        //ϵͳ���ļ����޸�
                        /*int sys_rootdir_open_flag=0;  //ϵͳ���ļ����Ƿ��Ѿ��򿪸�Ŀ¼�ļ��ı��
                        int pos_rootdir_sys_old;         //��Ŀ¼�ļ���ϵͳ�򿪱��е�λ��
                        for(l=0;l<SYSOPENFILE;l++)   //�ж�ϵͳ���ļ�������û�и�Ŀ¼�ļ��Ĵ򿪽��
                            {
                                if(sys_ofile[l].inode_no==0)
                                {
                                    sys_rootdir_open_flag=1;
                                    pos_rootdir_sys_old=i;
                                    //user_file_list[pos_userid].u_ofile
                                    for(k=0;k<NOFILE;k++)
                                        if(user_file_list[pos_userid].u_ofile[k]==-1)
                                            {
                                                user_file_list[pos_userid].u_ofile[k]=pos_rootdir_sys_old;  //д�û��򿪱��е�����
                                                break;
                                            }
                                    break;
                                }
                            }
                        if(sys_rootdir_open_flag==1)   //�Ѿ�����,��д�û��򿪱��е�����
                            sys_ofile[l].read_count++;   //���߼�1
                        else  //ϵͳ��ǰû�д򿪸�Ŀ¼�ļ������ڴ�
                        {
                            for(l=0;l<SYSOPENFILE;l++)
                            {


                                if(sys_ofile[l].inode_no==-1)   //ϵͳ�򿪱�򿪸�Ŀ¼�ļ�
                                {
                                    //�����û����ļ���
                                    for(k=0;k<NOFILE;k++)
                                    if(user_file_list[pos_userid].u_ofile[k]==-1)
                                    {
                                        user_file_list[pos_userid].u_ofile[k]=i;  //д�û��򿪱��е�����
                                        break;
                                    }

                                    sys_ofile[l].inode_no=0;
                                    sys_ofile[l].write_count=0;
                                    sys_ofile[l].read_count=1;
                                    sys_ofile[l].open_type=1;  //��ʽΪ��
                                    //�����ڴ�i���
                                    struct inode *inode_rootdir;
                                    inode_rootdir=(struct inode*)malloc(sizeof(struct inode));
                                    inode_rootdir->last=NULL;
                                    inode_rootdir->next=NULL;
                                    inode_rootdir->change_flag='n';

                                    inode_rootdir->file_size=5;  //����Ϊ���̵ĸ�Ŀ¼�ļ�i���(ֱ�Ӹ��Ƴ�ʼ��ʱ�ĸ�Ŀ¼���ɣ�
                                    inode_rootdir->inode_no=0;
                                    inode_rootdir->inode_type=0;
                                    inode_rootdir->i_mode=1;
                                    for(k=0;k<10;k++)
                                        inode_rootdir->i_addr[k]=-1;  //����Ϊ��ʱ����-1
                                    for(k=0;k<5;k++)    //��Ŀ¼�ļ�5�����ݿ������
                                        inode_rootdir->i_addr[k]=34+i;
                                    //�����ڴ�i�������
                                    insert_hinode(0,inode_rootdir);
                                    sys_ofile[l].f_inode=inode_rootdir;
                                    break;
                                }
                            }
                        }*/
                        //��Ŀ¼�����----------------------------------------------------------------------------------------
                        //open(root,userid);
                        break;
                    }

                if(j==10)
                {
                    printf("�Ѿ��ﵽ��ǰ����û���������10������¼ʧ�ܣ�");
                    return 0;
                }
                printf("��¼�ɹ���\n");
                login_flag=1;
                break;
            }
        if(login_flag==0)
            printf("��¼ʧ��\n");


    fclose(disk);                             //         Ϊʲô�������ͱ���
   /* for(i=0;i<10;i++)
        printf("(%d,%d)  ",user_file_list[i].user_id,user_file_list[i].group_id);*/
    return 0;
}

int logout(short userid)
{
    int i;
    //�رմ��ļ�
    //�ж��û��Ƿ��¼
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
        printf("�û�δ��¼������\n");
        return 0;
    }
    int num_samename=0;
    for(i=0;i<NOFILE;i++)   //�û�
    {//user_file_list[USERNUM]
        int index_sys=user_file_list[pos_user].u_ofile[i];   //�û����ļ���ϵͳ���е�����λ��
        if(sys_ofile[index_sys].inode_no!=-1)   //���˸��ļ�
        {
            int file_inode_no=sys_ofile[i].inode_no;   //����ļ�i���(ɾ��ʱ�ͷ�i�����������ã�
            //�޸�ϵͳ�򿪱�

            if(sys_ofile[index_sys].read_count+sys_ofile[index_sys].write_count==1)  //ֻ��һ���û���
            {

                strcpy(sys_ofile[index_sys].filename_open,"");
                sys_ofile[index_sys].read_count=0;
                sys_ofile[index_sys].read_count=0;
                sys_ofile[index_sys].inode_no=-1;

                //�ͷ��ڴ�i���
                if(hinode_index[file_inode_no%128].i_forw==sys_ofile[index_sys].f_inode)  //��ϣ�����Ӧ����λ�õ�һ��i���Ϊ��ǰ�ڴ�i���
                    hinode_index[file_inode_no%128].i_forw=NULL;
                else
                {
                    sys_ofile[index_sys].f_inode->last->next=sys_ofile[index_sys].f_inode->next;
                    if(sys_ofile[index_sys].f_inode->next!=NULL)
                        sys_ofile[index_sys].f_inode->next->last=sys_ofile[index_sys].f_inode->last;
                }
            }
            else   //�ж���û������ڶ�
                sys_ofile[index_sys].read_count--;

            //�޸��û���
            user_file_list[pos_user].u_ofile[i]=-1;
        }
        num_samename++;

    }

    //�û��ǳ�
    user_file_list[pos_user].user_id=-1;

    return 0;
}
int deleteuser(short delete_who,short who_delete)
{
    int i;
    //ɾ�����Ƿ�����
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
        printf("ɾ����δ��¼!!!\n");
        return 0;
    }
    //ɾ���߱����ǹ���Ա
    else
    {
        if(user_file_list[pos_who_delete].group_id!=0)   //���ǹ���Ա
            {
                printf("ɾ���߲��ǹ���Ա��û��ɾ��Ȩ�ޣ�����\n");
                return 0;
            }
    }
    //���Ǳ�ɾ�����Ƿ�����
    already_login_flag=0;
    int pos_user;   //�û��ļ�����λ��
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==delete_who)
            {
                already_login_flag=1;
                pos_user=i;
                break;
            }

    //������رմ��ļ�
    if(already_login_flag==1)   //Ҫɾ����
    {
        //�رմ��ļ�   >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>�����
        //��ɾ���û�����
        user_file_list[pos_user].user_id=-1;
    }
    //ɾ��
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    struct user_process user_list[PWDNUM];  //���̾�̬�û����ȡ    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);
    int exist_flag=0;
    int pos_delete_who;
    for(i=0;i<PWDNUM;i++)   //��ɾ�����Ƿ����
        if(user_list[i].user_id==delete_who)
            {
                exist_flag=1;
                pos_delete_who=i;
                break;
            }
    if(exist_flag==0)
    {
        printf("��ɾ���߲����ڣ�����\n");
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
int showalluser() //��ʾ�����е��û���
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�

    struct user_process user_list[PWDNUM];  //���̾�̬�û����ȡ    16*32=512B
    fseek(disk,39*512,0);
    fread(&user_list,512,1,disk);
    for(i=0;i<PWDNUM;i++)
        if(user_list[i].user_id!=-1)
        printf("ID=%d GROUP=%d password=%s\n",user_list[i].user_id,user_list[i].group_id,user_list[i].password);
    return 0;
}
int showonlineuser()   //��ʾ�����û��б�
{
    int i,count=0;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id!=-1)
            printf("�����û�%d userid=%d\n",++count,user_file_list[i].user_id);
    return 0;
}

int online(short userid)   //�ж��û��Ƿ����ߣ������򷵻��û����ļ����λ��,�����߷���-1
{
    int i;
    for(i=0;i<USERNUM;i++)
        if(user_file_list[i].user_id==userid)
                return i;     //�ѵ�¼
        return -1;            //δ��¼
}
int open(char *file_name,short userid)
{   //���ļ�
    /*
    ����˵�����򿪸�Ŀ¼�����˳�ʼ���û�Ŀ¼�����ݣ���������ļ��ֿ�д
                Ŀ¼�ļ�ֻ���Զ��ķ�ʽ��
                ��Ŀ¼�ļ�����ά���޸�ϵͳ����ڴ�i���
    */
    int i,j;
    FILE *disk;   //��ȡ��Ŀ¼�ļ���i���---------------------------------
    disk=fopen("disk","rb+");
    int pos_userid;  //�û����̱����û����̱���е�λ��
    int repeat_open_flag;  //�ظ��򿪱��
    char open_file_name[14];
    strcpy(open_file_name,file_name);
    pos_userid=online(userid);
    if(pos_userid==-1)
    {
        printf("�û�δ��¼\n");
        return 0;
    }
    //�ڵ�ǰĿ¼���������ļ������ҵ��ͻ�ȡi���ţ�Ϊ�ж��Ƿ��ظ����ļ���׼�����Ҳ����ͱ���
    int current_dir_inode_no;//��ǰĿ¼i���
    current_dir_inode_no=user_file_list[pos_userid].route_stack[user_file_list[pos_userid].route_stack_head-1];   //��ǰ�û�·��ջջջ��Ϊ��ǰĿ¼
    get_dirdetail(current_dir_inode_no);   //��ȡ���û���ǰĿ¼��ϸ��Ϣ//----bug_key2
    int file_type;    //Ҫ���ļ�������
    int file_inode_no;   //Ҫ���ļ���i����
    for(i=0;i<DIRNUM;i++)
    {    //�����ļ����ڵ�ǰĿ¼��Ѱ��
        if(current_dirdetail[i].inode_no!=-1&&(strcmp(current_dirdetail[i].filename,file_name)==0))
            {
                file_type=current_dirdetail[i].file_type;    //--------------   x=y  -1
                file_inode_no=current_dirdetail[i].inode_no;
                break;
            }
    }
    if(i==DIRNUM)//û���ҵ�
    {
        printf("�ļ�������!!!\n");
        return 0;
    }
    //�ж��Ƿ��ظ���
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
        printf("�ظ������ļ���Ŀ¼������\n");                //���Ե�1��δ����
        return 0;
    }

    //��Ŀ¼�ļ����ı��ļ�����������д�
    if(file_type==0)   //��Ŀ¼�ļ�,��Ŀ¼
    {  //�Զ��ķ�ʽ��
        //�û��ļ���(����·���洢��Ϣ���޸�
        int stack_head=user_file_list[pos_userid].route_stack_head;
        user_file_list[pos_userid].route_stack[stack_head]=file_inode_no;   //Ŀ¼i���ѹ��ջ��
        strcpy(user_file_list[pos_userid].dirname_stack[stack_head],file_name);   //Ŀ¼��ѹ��·����ջ
        user_file_list[pos_userid].route_stack_head++;
        //------------------------------����------------------
    //printf("%d->%d %d",user_file_list[pos_userid].route_stack_head,user_file_list[pos_userid].route_stack[0],user_file_list[pos_userid].route_stack[1]);

    }
    else    //���ı��ļ�,���ı�,��ȡ�ı�
    {
        //��д��ʽѡ��,���Ƕ�дȨ�����⣬change_flag�������ͷŷ�������ռ�
        //Ĭ���Զ��ķ�ʽ��
        //�޸Ĵ򿪷�ʽΪд�󱣳�д�ļ�״̬
        struct dinode file_dinode;

        fseek(disk,1024+32*file_inode_no,0);
        fread(&file_dinode,28,1,disk);
        int file_level=file_dinode.i_mode;   //�ļ�����
        int user_level=user_file_list[pos_userid].group_id;   //�û�����
        //printf("file_dinode.id=%d user_level=%d  file_level=%d\n",file_dinode.inode_no,user_level,file_level);
        if(user_level==1&&file_level==0)
        {
            printf("����Ա�����ļ�����ֹ�ǹ���Ա��!!!\n");
            fclose(disk);
            return 0;
        }
        int sys_file_open_flag=0;  //ϵͳ���ļ����Ƿ��Ѿ����ļ��ı��
        int pos_file_sys_old;         //���ļ���ϵͳ�򿪱��е�λ��
        for(i=0;i<SYSOPENFILE;i++)   //�ж�ϵͳ���ļ�������û��Ŀ¼�ļ��Ĵ򿪽��
        {
            if(sys_ofile[i].inode_no==file_inode_no)
            {   //ϵͳ�Ѵ򿪸�Ŀ¼�������û����̣�
                sys_file_open_flag=1;
                pos_file_sys_old=i;
                //user_file_list[pos_userid].u_ofile
                for(j=0;j<NOFILE;j++)
                    if(user_file_list[pos_userid].u_ofile[j]==-1)
                        {
                            user_file_list[pos_userid].u_ofile[j]=pos_file_sys_old;  //д�û��򿪱��е�����
                            break;
                        }
                if(j==NOFILE)
                {
                    printf("�û����ļ����ﵽ���ޣ�����\n");
                    return 0;
                }
                break;
            }
        }
        int current_sys_pos=i;   //ϵͳ�򿪱��е�λ�ã����ϵͳ���ˣ�
        printf("������д����w/r��:");
        char x[5];
        while(1)
        {
            scanf("%s",x);
            if(strcmp(x,"r")==0)
            {   //���ļ�
                if(sys_file_open_flag==1)   //ϵͳ�Ѿ���
                {
                    printf("������%d���û��ڶ�\n",sys_ofile[current_sys_pos].read_count);
                    printf("������%d���û���д\n",sys_ofile[current_sys_pos].write_count);
                    if(sys_ofile[current_sys_pos].write_count>0)
                    {
                        printf("���û�������д�������Զ��ķ�ʽ�򿪣�����\n");
                        //�ָ��û��򿪱��״̬
                        user_file_list[pos_userid].u_ofile[j]=-1;    //�����õ���ǰ��һ��ѭ���е���j
                        //�˳�
                        return 0;
                    }
                    else
                    {
                        //����ϵͳ���еĶ�����
                        sys_ofile[current_sys_pos].read_count++;

                        //������Կ�ʼ����!!!!!!!!!
                        if(file_dinode.file_size==0)
                        {
                            printf("1.����һ�����ļ���\n");
                        }
                        else   //���ǿ��ļ�
                        {
                            char *file_content;    //�洢��ȡ���ݵ�λ��
                            file_content=(char *)malloc(sizeof(char)*file_dinode.file_size+1);

                            int block_num=file_dinode.file_size/512+1;  //�洢���ļ����˼�����
                            int yushu=file_dinode.file_size%512;
                            if(yushu==0)
                                block_num++;   //�ļ�ռ���������


                            int *block_no;   //�洢������
                            block_no=(int *)malloc(sizeof(int)*block_num);
                            for(j=0;j<block_num;j++)
                            {
                                //ȥ�����ַ������ȡ�������
                                if(j<9)
                                    block_no[j]=file_dinode.i_addr[j];
                                else   //��ȡһ��������
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
                            int temp_size=file_dinode.file_size;   //ʣ���ȡ�ֽ���
                            int p_string=0;
                            for(j=0;j<block_num-1;j++)  //�������ȡ���ַ�����
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
                else    //ϵͳ��û�д�,  ���ڴ�
                {
                    //ϵͳ�򿪸��ļ�
                    for(i=0;i<SYSOPENFILE;i++)
                    {
                        if(sys_ofile[i].inode_no==-1)   //ϵͳ�򿪱��Ŀ¼�ļ�
                        {
                            //�����û����ļ���
                            for(j=0;j<NOFILE;j++)
                            if(user_file_list[pos_userid].u_ofile[j]==-1)
                            {
                                user_file_list[pos_userid].u_ofile[j]=i;  //д�û��򿪱��е�����
                                break;
                            }

                            sys_ofile[i].inode_no=file_inode_no;
                            sys_ofile[i].write_count=0;
                            sys_ofile[i].read_count=1;
                            sys_ofile[i].open_type=1;  //��ʽΪ��
                            strcpy(sys_ofile[i].filename_open,file_name);
                            //�����ڴ�i���
                            struct dinode dinode_file;

                            fseek(disk,1024+32*file_inode_no,0);
                            fread(&dinode_file,28,1,disk);
                            struct inode *inode_file;
                            inode_file=(struct inode*)malloc(sizeof(struct inode));
                            inode_file->last=NULL;
                            inode_file->next=NULL;
                            inode_file->change_flag='n';

                            inode_file->file_size=dinode_file.file_size;  //����Ϊ���̵�Ŀ¼�ļ�i���(Ĭ������(����i���ţ���
                            inode_file->inode_no=file_inode_no;
                            inode_file->inode_type=0;
                            inode_file->i_mode=dinode_file.i_mode;   //Ȩ��Ϊ�ͼ�
                            for(j=0;j<10;j++)
                                inode_file->i_addr[i]=dinode_file.i_addr[j];  //����Ϊ��ʱ����-1
                            //�����ڴ�i����ϣ����

                            insert_hinode(file_inode_no,inode_file);
                            sys_ofile[i].f_inode=inode_file;
                            break;
                        }
                    }
                    if(i==SYSOPENFILE)
                    {
                        printf("ϵͳ���ļ����ﵽ���ޣ�����\n");
                        return 0;
                    }
                    //������Կ�ʼ����!!!!!!!!!
                    //printf("(%d ��i���size=%d\n",file_dinode.inode_no,file_dinode.file_size);
                    if(file_dinode.file_size==0)
                    {
                        printf("2.����һ�����ļ���\n");   //>>>>>>>>>>>>>>>>>>>
                    }
                    else   //���ǿ��ļ�
                    {
                        char *file_content;    //�洢��ȡ���ݵ�λ��
                        file_content=(char *)malloc(sizeof(char)*file_dinode.file_size+1);

                        int block_num=file_dinode.file_size/512+1;  //�洢���ļ����˼�����
                        int yushu=file_dinode.file_size%512;
                        if(yushu==0)
                            block_num++;   //�ļ�ռ���������


                        int *block_no;   //�洢������
                        block_no=(int *)malloc(sizeof(int)*block_num);
                        for(j=0;j<block_num;j++)
                        {
                            //ȥ�����ַ������ȡ�������
                            if(j<9)
                                block_no[j]=file_dinode.i_addr[j];
                            else   //��ȡһ��������
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
                        int temp_size=file_dinode.file_size;   //ʣ���ȡ�ֽ���
                        int p_string=0;
                        for(j=0;j<block_num-1;j++)  //�������ȡ���ַ�����
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
                //д�ļ�
                if(sys_file_open_flag==1)   //ϵͳ�Ѿ���
                {
                    printf("������%d���û���д\n",sys_ofile[current_sys_pos].write_count);
                    printf("������%d���û��ڶ�\n",sys_ofile[current_sys_pos].read_count);
                    if(sys_ofile[current_sys_pos].read_count>0||sys_ofile[current_sys_pos].write_count>0)
                    {
                        printf("���û������ڶ�����д��������д�ķ�ʽ�򿪣�����\n");
                        //�ָ��û��򿪱��״̬
                        user_file_list[pos_userid].u_ofile[j]=-1;    //�����õ���ǰ��һ��ѭ���е���j
                        //�˳�
                        return 0;
                    }
                    else
                    {
                        //��ϵͳ�򿪱�ע��

                        sys_ofile[current_sys_pos].write_count++;

                        //������Կ�ʼд��!!!!!!!!!
                        //�ͷ������

                        //-------------------
                        //�ҿ��
                      /*  int block_num=file_dinode.file_size/512+1;  //�洢���ļ����˼�����
                        int yushu=file_dinode.file_size%512;
                        if(yushu==0)
                            block_num++;   //�ļ�ռ���������


                        int *block_no;   //�洢������
                        block_no=(int *)malloc(sizeof(int)*block_num);
                        for(j=0;j<block_num;j++)
                        {
                            //ȥ�����ַ������ȡ�������
                            if(j<9)
                                block_no[j]=file_dinode.i_addr[j];
                            else   //��ȡһ��������
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
                        release_block(file_dinode.i_addr);  //addr�ڸú������ÿ�

                        //------------------
                        printf("�����ı����ݣ�");
                        char file_content_write[1000];  //Ҫ������ı�
                        scanf("%s",file_content_write);
                    //gets(file_content_write);
                        int file_size=strlen(file_content_write);   //�ļ��ֽ���
                        file_dinode.file_size=file_size;

                        int block_num=file_size/512+1;  //�洢���ļ��ü�����
                        int yushu=file_size%512;
                        if(yushu==0)
                            block_num++;   //�ļ�ռ���������
                        giveblock(file_dinode.i_addr,block_num);
                        //��ȡ���
                        int *block_no;   //�洢������
                        block_no=(int *)malloc(sizeof(int)*block_num);
                        for(j=0;j<block_num;j++)
                        {
                            //ȥ�����ַ������ȡ�������
                            if(j<9)
                                block_no[j]=file_dinode.i_addr[j];
                            else   //��ȡһ��������
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
                        //д�����
                        int temp_size=file_dinode.file_size;   //ʣ��д���ֽ���
                        int p_string=0;
                        for(j=0;j<block_num-1;j++)  //�������ȡ���ַ�����
                        {
                            fseek(disk,block_no[j]*512,0);
                            fwrite(&file_content_write[p_string],512,1,disk);
                            p_string+=512;
                            temp_size-=512;
                        }
                        fseek(disk,block_no[block_num-1]*512,0);
                        fwrite(&file_content_write[p_string],temp_size,1,disk);   //???????????????
                        printf("1.д��ɹ�\n");
                    }
                }
                else     //ϵͳ��û�д�
                {
                    //ϵͳ�򿪸��ļ�
                    for(i=0;i<SYSOPENFILE;i++)
                    {
                        if(sys_ofile[i].inode_no==-1)   //ϵͳ�򿪱��Ŀ¼�ļ�
                        {
                            //�����û����ļ���
                            for(j=0;j<NOFILE;j++)
                            if(user_file_list[pos_userid].u_ofile[j]==-1)
                            {
                                user_file_list[pos_userid].u_ofile[j]=i;  //д�û��򿪱��е�����
                                break;
                            }

                            sys_ofile[i].inode_no=file_inode_no;
                            sys_ofile[i].write_count=1;
                            sys_ofile[i].read_count=0;
                            sys_ofile[i].open_type=0;  //��ʽΪд
                            strcpy(sys_ofile[i].filename_open,file_name);
                            //�����ڴ�i���
                            struct dinode dinode_file;

                            fseek(disk,1024+32*file_inode_no,0);
                            fread(&dinode_file,28,1,disk);
                            struct inode *inode_file;
                            inode_file=(struct inode*)malloc(sizeof(struct inode));
                            inode_file->last=NULL;
                            inode_file->next=NULL;
                            inode_file->change_flag='n';

                            inode_file->file_size=dinode_file.file_size;  //����Ϊ���̵�Ŀ¼�ļ�i���(Ĭ������(����i���ţ���
                            inode_file->inode_no=file_inode_no;
                            inode_file->inode_type=0;
                            inode_file->i_mode=dinode_file.i_mode;
                            for(j=0;j<10;j++)
                                inode_file->i_addr[i]=dinode_file.i_addr[j];
                            //�����ڴ�i����ϣ����

                            insert_hinode(file_inode_no,inode_file);
                            sys_ofile[i].f_inode=inode_file;
                            break;
                        }
                    }
                    if(i==SYSOPENFILE)
                    {
                        printf("ϵͳ���ļ����ﵽ���ޣ�����\n");
                        return 0;
                    }
                    //������Կ�ʼд��!!!!!!!!!
                    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
                    //������Կ�ʼд��!!!!!!!!!
                    //�ͷ������

                    //-------------------
                    //�ҿ��
               /*     int block_num=file_dinode.file_size/512+1;  //�洢���ļ����˼�����
                    int yushu=file_dinode.file_size%512;
                    if(yushu==0)
                        block_num++;   //�ļ�ռ���������


                    int *block_no;   //�洢������
                    block_no=(int *)malloc(sizeof(int)*block_num);
                    for(j=0;j<block_num;j++)
                    {
                        //ȥ�����ַ������ȡ�������
                        if(j<9)
                            block_no[j]=file_dinode.i_addr[j];
                        else   //��ȡһ��������
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
                    release_block(file_dinode.i_addr);  //addr�ڸú������ÿ�

                    //------------------
                    printf("�����ı����ݣ�");
                    char file_content_write[1000];  //Ҫ������ı�
                    scanf("%s",file_content_write);
                    //gets(file_content_write);
                    int file_size=strlen(file_content_write);   //�ļ��ֽ���
                    file_dinode.file_size=file_size;

                    printf("(%d ��i���size=%d\n",file_dinode.inode_no,file_dinode.file_size);
                    int block_num=file_size/512+1;  //�洢���ļ��ü�����
                    int yushu=file_size%512;
                    if(yushu==0)
                        block_num++;   //�ļ�ռ���������
                    giveblock(file_dinode.i_addr,block_num);
                    //��ȡ���
                    int *block_no;   //�洢������
                    block_no=(int *)malloc(sizeof(int)*block_num);
                    for(j=0;j<block_num;j++)
                    {
                        //ȥ�����ַ������ȡ�������
                        if(j<9)
                            block_no[j]=file_dinode.i_addr[j];
                        else   //��ȡһ��������
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
                    //д�����
                    int temp_size=file_dinode.file_size;   //ʣ��д���ֽ���
                    int p_string=0;
                    for(j=0;j<block_num-1;j++)  //�������ȡ���ַ�����
                    {
                        fseek(disk,block_no[j]*512,0);
                        fwrite(&file_content_write[p_string],512,1,disk);
                        p_string+=512;
                        temp_size-=512;
                    }
                    fseek(disk,block_no[block_num-1]*512,0);
                    fwrite(&file_content_write[p_string],temp_size,1,disk);   //???????????????
                    printf("2.д��ɹ�\n");
                    //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
                    //i���д��
                    fseek(disk,file_dinode.inode_no*32+1024,0);

                    fwrite(&file_dinode,28,1,disk);


                }  //ϵͳû��ʱд���
                break;   //����Ҫ�޸ĵ�ǰĿ¼
            }
            else
            {

                printf("����w/r���������룺\n");
            }
        }
        getchar();

    }

    fclose(disk);
    return 0;
}

int insert_hinode(short inode_no,struct inode* inode_insert)   //�ڴ�i����ϣ����Ĳ���
{
     int insert_pos;   //�ڹ�ϣ�����еĲ���λ��
     insert_pos=inode_no%128;  //��ϣ����

     if(hinode_index[insert_pos].i_forw!=NULL)  //������λ�ò�Ϊ��
     {
            struct inode* x=hinode_index[insert_pos].i_forw;
            while(x->next!=NULL)
                x=x->next;
            x->next=inode_insert;   //��ʱx�Ѿ�ָ�����һ��
            (x->next)->last=x;
    }
    else    //������λ��Ϊ��
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
        printf("�û�δ��¼!!!\n");
        return 0;
    }
    for(i=0;i<user_file_list[pos].route_stack_head;i++)   //��ջ��ÿһ��useridȡ�ļ���
        printf("/%s",user_file_list[pos].dirname_stack[i]);
    printf("\n");
    return 0;
}

int giveblock_single()  //���䵥������飨Ϊ���������ʱʹ�ã�
{
    int block_return;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    if(internal_filsys.freeblock_stack_num>1)  //��ǰջ�д���1�����п�
    {
        block_return=internal_filsys.freeblock_stack[--internal_filsys.freeblock_stack_num];   //���ջ�����
        internal_filsys.changed='y';
    }
    else
    {
        int block_manage;//�鳤����
        //if(internal_filsys.freeblock_stack[0]!=-1)��Ȼ���������жԿ��п���������֤�Ͳ���Ҫ����ж���

        block_manage=internal_filsys.freeblock_stack[0];
        //��ȡ�鳤�鵽���п�ջ
        int offset_block_manage=512*block_manage;
        fseek(disk,offset_block_manage,0);
        fread(&internal_filsys.freeblock_stack[0],200,1,disk);   //�Ĵ�
        internal_filsys.changed='y';
        internal_filsys.freeblock_stack_num=50;  //�Ĵ�
        block_return=block_manage;
    }
    fclose(disk);
    return block_return;
}
int release_block_single(int block_no)
{   //�ͷŵ�����
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    if(internal_filsys.freeblock_stack_num<50)  //��ǰջû��
        internal_filsys.freeblock_stack[internal_filsys.freeblock_stack_num++];  //ѹջ
    else
    {
        //�ͷŵĿ�д���鳤��
        struct block_grouphead  group_head;
        for(i=0;i<50;i++)
            group_head.block_index[i]=internal_filsys.freeblock_stack[i];

        fseek(disk,block_no*512,0);
        fwrite(&group_head,100,1,disk);


        //�������д
        internal_filsys.freeblock_stack[0]=block_no;  //Ҫ�ͷŵĿ���Ϊ�鳤��
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
{    //�ͷſ�(��Ҫ��֤file_
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    int need_release_block[265];  //�ݴ���Ҫ�ͷŵĿ��
    int need_release_size=0;   //��Ҫ�ͷŵĿ���
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
            //����������Ϊ�գ�˵���Ѿ��ͷ����
        }
        else
        {
            struct block_grouphead this_indexblock;  //������
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
{   //Ϊ�ļ�������̿�   ��ʹ��һ�μ������������addr[0]-addr[8]���ֱ��������addr[9]���һ�μ��������
    int i,j;
    int real_block_num=num_block;  //��������Ŀ��������������飩
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    if(num_block>9)
        real_block_num++;
    if(num_block==0)
    {    //�������
        for(i=0;i<10;i++)
            addr[i]=-1;
        return 0;
    }
    if(real_block_num>internal_filsys.freeblock_num)    //Ҫ����Ŀ������ڵ�ǰʣ���ܿ���
    {
        printf("ʣ�����ݿ鲻�㣡����\n");
        return 0;
    }
    if(num_block>265)    //Ҫ����Ŀ������������������
    {
        printf("Ҫ����Ŀ������������������������\n");
        return 0;
    }

    //�Ȼ�ȡ����Ĵ��̿��

    short block_no[511];
    for(i=0;i<num_block;i++)
    {

        if(internal_filsys.freeblock_stack_num>1)  //��ǰջ�д���1�����п�
        {
            block_no[i]=internal_filsys.freeblock_stack[--internal_filsys.freeblock_stack_num];   //���ջ�����
            internal_filsys.changed='y';
        }
        else  //internal_filsys.freeblock_stack_num=1;
        {
            int block_manage;//�鳤����
            if(internal_filsys.freeblock_stack[0]!=-1)
            {
                block_manage=internal_filsys.freeblock_stack[0];
                //��ȡ�鳤�鵽���п�ջ
                int offset_block_manage=512*block_manage;
                fseek(disk,offset_block_manage,0);
                fread(&internal_filsys.freeblock_stack[0],200,1,disk);   //�Ĵ�
                internal_filsys.changed='y';
                internal_filsys.freeblock_stack_num=50;  //�Ĵ�
                block_no[i]=block_manage;
            }
            else
            {   //֮ǰ�Ѿ��������жϣ���β���ִ�У���׼�㷨��û��֮ǰ���ڿ鹻�������жϵģ����ǵ���ʱ�Ὣ�����������������ǵĳ���û�н����������ƣ������ǰ�жϿ鹻����
                printf("ʣ�����ݿ鲻�㣡����\n");
                return 0;
            }

        }
    }
    internal_filsys.freeblock_num-=num_block;
    //�Ѵ��̿������i�������������
    if(num_block<10)  //С��ʮ�鲻�ö༶����
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
        //����һ��������
        int index_block_no=giveblock_single();
        struct index_block index_block_current;
        for(j=0;j<256;j++)
            index_block_current.index[j]=-1;  //�������ʼ��
        for(i=9;i<num_block;i++)
            index_block_current.index[i-9]=block_no[i];
        //������д��size����
        fseek(disk,index_block_no*512,0);
        fwrite(&index_block_current,512,1,disk);
    }
    fclose(disk);
    return 0;
}
int mkdir(char *dirname,short userid)
{    //�ڵ�ǰĿ¼���½�һ��Ŀ¼
    //ע���½�Ŀ¼ʱ��Ŀ¼��ĳ�ʼ�����ÿգ�
    //ע����Ŀ¼����
    int i;
    int pos_user=online(userid);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    short new_inode_no=new_inode();
    //��Ŀ¼ջջ��ͨ��i���ŷ��ʴ����ҵ���ǰĿ¼�ļ�����Ŀ¼�������������i���ź�Ŀ¼��
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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


  /*  fseek(disk,offset_fatherdir_data,0);   //------------------���Ե㣬��Ϊ��Ŀ¼�ļ��������ݲ�����������������ɢ��
    fread(&curret_dir,2052,1,disk);*/
    if(curret_dir.size>(DIRNUM-1))
    {
        printf("��ǰĿ¼Ŀ¼�����ﵽ���ޣ�����\n");
        return 0;
    }
    //���Ŀ¼����(Ŀ¼�����ļ���Ҳ�����ظ�
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,dirname)==0)
        {
            printf("1.Ŀ¼���ļ���������������ʧ�ܣ�����\n");
            return 0;
        }

    }
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no==-1)  //�ҵ���Ŀ¼��
        {
            curret_dir.direct_list[i].inode_no=new_inode_no;
            strcpy(curret_dir.direct_list[i].file_name,dirname);
            break;
        }
    }
    curret_dir.size++;   //��ǰĿ¼��������1
    //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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
    fwrite(&curret_dir,2052,1,disk);   //���޸ĺ�ĵ�ǰĿ¼�ļ�д��������*/
    //�Ӵ���ȡ�������i��㣬�����ļ�����ΪĿ¼�ļ�,�Լ�i������������
    struct dinode new_created_dinode;                //---------------------------����λ��
    int offset_new_dinode=1024+new_inode_no*32;
    new_created_dinode.inode_no=new_inode_no;
    new_created_dinode.inode_type=0;
    new_created_dinode.i_mode=1;   //Ŀ¼�ļ��Ķ�дȨ��Ĭ��Ϊ�ͼ�
    new_created_dinode.file_size=5;  //Ŀ¼�ļ���СΪ5
    giveblock(new_created_dinode.i_addr,5);  //�����   -------------------------�����1��������ʱ����������дΪ��
    fseek(disk,offset_new_dinode,0);
    fwrite(&new_created_dinode,28,1,disk);  //д����Ŀ¼��i���
    //����Ŀ¼�ļ�д��new_created_dinode.i_addr��Ӧ�������ַ
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


    //ѯ���Ƿ�����½�Ŀ¼
    printf("�Ƿ�����½�Ŀ¼(y/n)");
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

            break;   //����Ҫ�޸ĵ�ǰĿ¼
        }
        else
        {

            printf("�������룺\n");
        }
    }
getchar();
    return 0;
}


short new_inode()
{    //����ֻ��������µ�i���Ų��޸ĳ����飨�ڴ��еģ����½����ĳ�ʼ���������������н��С�
    int i;
    if(internal_filsys.freeinode_stack_num>0)
    {
        //internal_filsys.inode_num--;
        internal_filsys.changed='y';
        int x=(--internal_filsys.freeinode_stack_num);
        return internal_filsys.freeinode_stack[x];
    }
    else    //����i���ջ��
    {
        int have_freeinode_flag=0;
        struct dinode temp_dinode;

        for(i=internal_filsys.flag_inode;i<512;i++)   //������i��㿪ʼ�ҿ���i���
        {
            iget(i,&temp_dinode);
            if(temp_dinode.inode_type==-1)
            {
                have_freeinode_flag=1;
                internal_filsys.freeinode_stack[internal_filsys.freeinode_stack_num++]=temp_dinode.inode_no;  //��ջ
                if(internal_filsys.freeinode_stack_num==50)   //ջ��
                {

                    int y=(--internal_filsys.freeinode_stack_num);
                    return internal_filsys.freeinode_stack[y];
                }
            }
        }
        if(have_freeinode_flag==0&&i==512)
        {
            //internal_filsys.changed='n';   //ֻ����޸ģ�����ǲ��޸�
            printf("û�п���i��㣬��ر�һЩ�ļ��ٴ򿪡�\n");
            return 0;
        }
        //ʣ��i��㲻��50�������
        int z=(--internal_filsys.freeinode_stack_num);
        internal_filsys.changed='y';
        return internal_filsys.freeinode_stack[z];
    }
    return 0;
}

int get_new_inode()   //�½�i�����Ժ���
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
{     //�ͷŴ���i���(��Ҫ��i�������,i���Ÿ�Ϊ��д�أ�
    struct dinode empty_dinode;
    empty_dinode.inode_type=-1;
    empty_dinode.inode_no=dinode_no;
    if(dinode_no<internal_filsys.flag_inode)   //Ҫ�ͷŵ�i����С������i���
    {
        if(internal_filsys.freeinode_stack_num==50)   //ջ�������ѿ���i���д�ش��̲��޸Ŀ���i���ջ
            {
                internal_filsys.flag_inode=dinode_no;   //�޸�����i���
                internal_filsys.changed='y';
                iput(dinode_no,&empty_dinode);   //д�ؿ���i���
            }
        else
            {
                internal_filsys.changed='y';
                iput(dinode_no,&empty_dinode);   //д�ؿ���i���
                internal_filsys.freeinode_stack[internal_filsys.freeinode_stack_num++]=dinode_no;  //��ջ
            }
    }
    else
    {
        if(internal_filsys.freeinode_stack_num!=50)   //ջ����
            {
                internal_filsys.changed='y';
                iput(dinode_no,&empty_dinode);   //д�ؿ���i���
                internal_filsys.freeinode_stack[internal_filsys.freeinode_stack_num++]=dinode_no;  //��ջ
            }
        else  //ջ������д�ؿ���i���
            iput(dinode_no,&empty_dinode);   //д�ؿ���i���;
    }
    return 0;
}
int read_superblock()            //��һ��д��ʱ���������ˣ���ʽ�������
{
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    fseek(disk,512,0);
    fread(&internal_filsys,315,1,disk);
    internal_filsys.changed='n';
    return 0;
}
int write_superblock()  //д�س�����
{
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    fseek(disk,512,0);
    fwrite(&internal_filsys,315,1,disk);
    fclose(disk);
    printf("д�س�����\n");
    return 0;
}
int show_empty_inode()  //��ʾ����i���ջ����
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
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    int i,j;
    //д�س�����
    printf("2.internal_filsys.changed=%c\n",internal_filsys.changed);
    if(internal_filsys.changed=='y')
        write_superblock();//д�س�����

    //�ڴ�i���д�ش���i���
    for(i=0;i<NHINO;i++)
    {
        struct dinode write_back_dinode;
        struct hinode current_hinode;   //�����ڹ�ϣ�����б���
        current_hinode.i_forw=hinode_index[i].i_forw;
        if(hinode_index[i].i_forw!=NULL)
        {
            while(1)
            {
                if(current_hinode.i_forw->change_flag=='y')
                {
                    //д��i���

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

int get_dirdetail(int currentdir_inode_no)   //��õ�ǰĿ¼����ϸ��Ϣ     -----bug_key3
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    struct dinode current_dir_dinode;
    iget(currentdir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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
        //д��ǰĿ¼��ϸ��ϢΪ��
        current_dirdetail[i].inode_no=-1;   //����һ�β��ҽ�������
        if(curret_dir.direct_list[i].inode_no!=-1)
        {
            current_dirdetail[i].inode_no=curret_dir.direct_list[i].inode_no;
            strcpy(current_dirdetail[i].filename,curret_dir.direct_list[i].file_name);
            //��Ŀ¼���i�����ҵ��ļ�����
            fseek(disk,1024+32*current_dirdetail[i].inode_no,0);
            fread(&temp,28,1,disk);    //--------------------------��Ϊfread
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
        printf("�û�����δ��¼��\n");
        return 0;
    }
    //printf("inode_no_fatherdir=%d\n",dir_inode_no);
    printf("\n");
    get_dirdetail(dir_inode_no);
    showroute(userid);  //�����ǰ·��
    printf("-------------------------------------------------------------\n");
    printf("���          �ļ���         i����                 �ļ�����|\n");
    for(i=0;i<DIRNUM;i++)
        if(current_dirdetail[i].inode_no!=-1)
        {
            if(current_dirdetail[i].file_type==1)
             {
                 printf("%d             %s             %d                    ",j++,current_dirdetail[i].filename,current_dirdetail[i].inode_no);
                 printf("�ı��ļ�|\n");
             }
            else
            {
                printf("%d             %s             %d                    ",j++,current_dirdetail[i].filename,current_dirdetail[i].inode_no);
                printf("  Ŀ¼|\n");
            }
        }
    printf("-------------------------------------------------------------\n");
    return 0;
}
int back_dir(short userid)
{
    int pos_user=online(userid);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    if(user_file_list[pos_user].route_stack_head==1)
    {
        printf("��ֹ�˳���Ŀ¼!1!\n");
        return 0;
    }
    user_file_list[pos_user].route_stack_head--;
    return 0;
}
int create(char *filename,short userid)
{
    int i;
    int pos_user=online(userid);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    short new_inode_no=new_inode();
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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
        printf("��ǰĿ¼Ŀ¼�����ﵽ���ޣ�����\n");
        return 0;
    }
    //���Ŀ¼����(Ŀ¼�����ļ���Ҳ�����ظ�
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,filename)==0)
        {
            printf("2.Ŀ¼���ļ���������������ʧ�ܣ�����\n");
            return 0;
        }

    }
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no==-1)  //�ҵ���Ŀ¼��
        {
            curret_dir.direct_list[i].inode_no=new_inode_no;
            strcpy(curret_dir.direct_list[i].file_name,filename);
            break;
        }
    }
    curret_dir.size++;   //��ǰĿ¼��������1
    //���޸ĺ��Ŀ¼�ļ�д�ش���
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

    //i���ķ����д��
    struct dinode new_created_dinode;                //---------------------------����λ��
    int offset_new_dinode=1024+new_inode_no*32;
    new_created_dinode.inode_no=new_inode_no;
    new_created_dinode.inode_type=1;
    new_created_dinode.i_mode=1;   //�ı��ļ��Ķ�дȨ��Ĭ��Ϊ�ͼ�
    new_created_dinode.file_size=0;  //���ļ�
    giveblock(new_created_dinode.i_addr,0);  //�����   -------------------------�����1��������ʱ����������дΪ��
    fseek(disk,offset_new_dinode,0);
    fwrite(&new_created_dinode,28,1,disk);  //д����Ŀ¼��i���

    fclose(disk);
    return 0;
}
int upfile(char *filename,short userid)   //����Աȡ�õ�ǰĿ¼��ĳ���ļ�������Ȩ
{
    int i;
    int pos_user=online(userid);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    if(user_file_list[pos_user].group_id==1)
    {
        printf("��ֹ�ǹ���Ա��ù���Ա����Ȩ������\n");
        return 0;
    }
    int file_inode_no;  //Ҫ�������ļ���
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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


    //����ļ��Ƿ����
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,filename)==0)
        {
            file_inode_no=curret_dir.direct_list[i].inode_no;   //���Ҫ�����ļ���i����
            break;
        }

    }
    if(i==DIRNUM)
    {
        printf("�ļ������ڣ�����\n");
        return 0;
    }
    struct dinode new_dinode;
    fseek(disk,1024+32*file_inode_no,0);
    fread(&new_dinode,28,1,disk);
    new_dinode.i_mode=0;
    fseek(disk,1024+32*new_dinode.inode_no,0);
    fwrite(&new_dinode,28,1,disk);   //�������i���д�ش���
    fclose(disk);
    return 0;
}

int downfile(char *filename,short userid)   //����Աȡ����ǰĿ¼��ĳ���ļ�������Ȩ
{
    int i;
    int pos_user=online(userid);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    if(user_file_list[pos_user].group_id==1)
    {
        printf("��ֹ�ǹ���Ա��ù���Ա����Ȩ������\n");
        return 0;
    }
    int file_inode_no;  //Ҫ�������ļ���
    short current_dir_inode_no;
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];  //-------1
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    struct dinode current_dir_dinode;
   // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
    //fread(&current_dir_dinode,28,1,disk);

    iget(current_dir_inode_no,&current_dir_dinode);

    struct dir curret_dir;
    //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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


    //����ļ��Ƿ����
    for(i=0;i<DIRNUM;i++)
    {
        if(curret_dir.direct_list[i].inode_no!=-1&&strcmp(curret_dir.direct_list[i].file_name,filename)==0)
        {
            file_inode_no=curret_dir.direct_list[i].inode_no;   //���Ҫ�����ļ���i����
            break;
        }

    }
    if(i==DIRNUM)
    {
        printf("�ļ������ڣ�����\n");
        return 0;
    }
    struct dinode new_dinode;
    fseek(disk,1024+32*file_inode_no,0);
    fread(&new_dinode,28,1,disk);
    new_dinode.i_mode=1;
    printf("id=%d level=%d\n",new_dinode.inode_no,new_dinode.i_mode);
    fseek(disk,1024+32*new_dinode.inode_no,0);
    fwrite(&new_dinode,28,1,disk);   //�������i���д�ش���
    fclose(disk);
    return 0;
}
int close(char *filename,short userid)
{
    int i;
    int pos_user=online(userid);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    int num_samename=0;

    for(i=0;i<NOFILE;i++)   //�û�
    {//user_file_list[USERNUM]
        int index_sys=user_file_list[pos_user].u_ofile[i];   //�û����ļ���ϵͳ���е�����λ��
        if(strcmp(sys_ofile[index_sys].filename_open,filename)==0)
        {
            if(sys_ofile[index_sys].inode_no!=-1)   //˫���գ�ȷ���Ѿ���
            {
                int file_inode_no=sys_ofile[i].inode_no;   //����ļ�i���(ɾ��ʱ�ͷ�i�����������ã�
                //�޸�ϵͳ�򿪱�

                if(sys_ofile[index_sys].read_count+sys_ofile[index_sys].write_count==1)  //ֻ��һ���û���
                {

                    strcpy(sys_ofile[index_sys].filename_open,"");
                    sys_ofile[index_sys].read_count=0;
                    sys_ofile[index_sys].read_count=0;
                    sys_ofile[index_sys].inode_no=-1;

                    //�ͷ��ڴ�i���
                    if(hinode_index[file_inode_no%128].i_forw==sys_ofile[index_sys].f_inode)  //��ϣ�����Ӧ����λ�õ�һ��i���Ϊ��ǰ�ڴ�i���
                        hinode_index[file_inode_no%128].i_forw=NULL;
                    else
                    {
                        sys_ofile[index_sys].f_inode->last->next=sys_ofile[index_sys].f_inode->next;
                        if(sys_ofile[index_sys].f_inode->next!=NULL)
                            sys_ofile[index_sys].f_inode->next->last=sys_ofile[index_sys].f_inode->last;
                    }
                }
                else   //�ж���û������ڶ�
                    sys_ofile[index_sys].read_count--;






                //�޸��û���
                user_file_list[pos_user].u_ofile[i]=-1;
            }
            num_samename++;
        }
    }
    if(num_samename<2)
        printf("�رճɹ���\n");
    else
        printf("�ر�%d��ͬ���ļ�\n",num_samename);
    return 0;
}

int deletefile(char *filename,short userid)
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    int pos_user=online(userid);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    //��ǰĿ¼���������ļ����ж��Ƿ���ڣ���ȡi����;��ȡ�ļ�����
    int current_dir_inode_no;//��ǰĿ¼i���
    current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];   //��ǰ�û�·��ջջջ��Ϊ��ǰĿ¼
    get_dirdetail(current_dir_inode_no);   //��ȡ���û���ǰĿ¼��ϸ��Ϣ//----bug_key2
    int file_type;    //Ҫ���ļ�������
    int file_inode_no;   //Ҫɾ���ļ���i����
    for(i=0;i<DIRNUM;i++)
    {    //�����ļ����ڵ�ǰĿ¼��Ѱ��
        if(current_dirdetail[i].inode_no!=-1&&(strcmp(current_dirdetail[i].filename,filename)==0))
            {
                file_type=current_dirdetail[i].file_type;    //--------------   x=y  -1
                file_inode_no=current_dirdetail[i].inode_no;
                break;
            }
    }
    if(i==DIRNUM)//û���ҵ�
    {
        printf("�ļ�������!!!\n");
        return 0;
    }
    //�ж��ļ�����
    if(file_type==1)  //ɾ���ı��ļ�
    {
        //�ж��Ƿ�ϵͳ��
        int sys_file_open_flag=0;  //ϵͳ���ļ����Ƿ��Ѿ����ļ��ı��

        for(i=0;i<SYSOPENFILE;i++)   //�ж�ϵͳ���ļ�������û��Ŀ¼�ļ��Ĵ򿪽��
        {
            if(sys_ofile[i].inode_no==file_inode_no)
            {   //ϵͳ�Ѵ򿪸�Ŀ¼�������û����̣�
                sys_file_open_flag=1;
                break;
            }
        }
        if(sys_file_open_flag==1)
        {
            printf("�ļ��Ѿ����򿪣���ֹɾ��!!!\n");
            return 0;
        }
        //��ʼɾ��
        //1.��ȡi���
        FILE *disk;
        disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
        fseek(disk,file_inode_no*32+1024,0);
        struct dinode file_dinode;
        fread(&file_dinode,28,1,disk);
        release_block(file_dinode.i_addr);
        release_dinode(file_inode_no);

        //ɾ��Ŀ¼�ļ��ж�ӦĿ¼��
        struct dinode current_dir_dinode;
       // int offset_fatherdir_inode=1024+current_dir_inode_no*32;
        //fread(&current_dir_dinode,28,1,disk);

        iget(current_dir_inode_no,&current_dir_dinode);

        struct dir curret_dir;
        //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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

        //�޸�Ŀ¼��


        for(i=0;i<DIRNUM;i++)
            if(curret_dir.direct_list[i].inode_no==file_inode_no)
        {
            curret_dir.direct_list[i].inode_no=-1;
            strcpy(curret_dir.direct_list[i].file_name,"");
            break;
        }
        curret_dir.size--;   //��ǰĿ¼��������1
        //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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
    else  //ɾ��Ŀ¼�ļ�
    {
        //Ҫɾ��Ŀ¼��i��� = file_inode_no
        //�����Ŀ¼
        user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head++]=file_inode_no;
        //����ɾ��ÿһ��Ŀ¼���Ӧ���ļ�
        //1����ȡĿ¼�ļ�
        struct dinode current_dir_dinode;
        int deep_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];
        //��ȡ����Ŀ¼��Ŀ¼�ļ�
        iget(deep_dir_inode_no,&current_dir_dinode);

        struct dir curret_dir;
        //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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
        //�˳���Ŀ¼
        user_file_list[pos_user].route_stack_head--;
        //�޸ĺ�д��Ŀ¼�ļ�
        int current_dir_inode_no=user_file_list[pos_user].route_stack[user_file_list[pos_user].route_stack_head-1];
        //��ȡ��ʼĿ¼��Ŀ¼�ļ�
        iget(current_dir_inode_no,&current_dir_dinode);


        //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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
        curret_dir.size--;   //��ǰĿ¼��������1
        //�Ӹ�������λ�ö�ȡ��ǰĿ¼�ļ�������
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

int upuser(short userid_uped,short userid_up)  //������ͨ�û�Ϊ����Ա�û�
{
    int i;
    FILE *disk;
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    int pos_user=online(userid_up);   //���û��򿪱���е�λ��
    if(pos_user==-1)
    {
        printf("�û�����δ��¼��\n");
        return 0;
    }
    if(user_file_list[pos_user].group_id==1)
    {
        printf("��ֹ�ǹ���Աִ����Ȩ����������\n");
        return 0;
    }
    //��Ȩ
    int pos_usered=online(userid_uped);   //���û��򿪱���е�λ��
    //1.�ڴ��û����޸�
    if(pos_usered!=-1)   //�û���¼��
        user_file_list[pos_usered].group_id=0;
    //2.�����û����޸�
    struct user_process user_list[PWDNUM];  //���̾�̬�û����ȡ    16*32=512B
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
    disk=fopen("disk","rb+");//�ļ�ָ��ָ������ļ�
    int i,j;
    printf("\n        --------------------��ӭ����UNIX VFS ϵͳ--------------------\n\n\n");
    printf("�Ƿ�Ҫ��ʽ������y/n):");
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
            printf("�������");
        }
    }
    read_superblock();
    user_file_list_initialise();  //�û����ļ�����ĳ�ʼ��
    sys_ofile_initialise() ;     //ϵͳ���ļ����ʼ��
    hinode_index_initialise();   //�ڴ�i����ϣ����������ĳ�ʼ��
    char order[50];
    char order_cutup[10][50];  //��������������һ���ַ���Ϊ������
    int order_cutup_i;  //�����ֳɵ��ַ�������
    int len;  //�����
    while(1)
    {
        j=0;
        order_cutup_i=1;   //�����ֳɵ��ַ�������
        printf("�������");
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
        order_cutup[order_cutup_i-1][j]='\0';//���һ��������β���������޲�������β����
        if(strcmp(order_cutup[0],"login")==0)
            {
                if(order_cutup_i!=3)   //��������
                    {
                        printf("�����������\n");
                        continue;
                    }
                login(order_cutup[1],order_cutup[2]);
            }
        else if(strcmp(order_cutup[0],"logout")==0)
            logout(short(atoi(order_cutup[1])));
        else if(strcmp(order_cutup[0],"showalluser")==0)
            showalluser();    //��ʾ������������û�
        else if(strcmp(order_cutup[0],"showonlineuser")==0)
            showonlineuser();    //��ʾ�����û�
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
        else if(strcmp(order_cutup[0],"mkdir")==0)   //�½�Ŀ¼�ļ�
            mkdir(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"create")==0)   //�½�Ŀ¼�ļ�
            create(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"upfile")==0)   //�½�Ŀ¼�ļ�
            upfile(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"downfile")==0)   //�½�Ŀ¼�ļ�
            downfile(order_cutup[1],short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"upuser")==0)   //�½�Ŀ¼�ļ�
            upuser(short(atoi(order_cutup[1])),short(atoi(order_cutup[2])));
        else if(strcmp(order_cutup[0],"close")==0)   //�½�Ŀ¼�ļ�
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
            printf("�����������󣡣���\n");
        }

    }
    fclose(disk);
    return 0;
}
