//1.lib
#include <iostream>
#include <cstdio>
#include <utility>
#include <vector>
#include <sstream>
#include <cstring>
//2.pre-set err
#define Unknown_Command "Unknown Command.\n"//未识别出是ls、ls -l还是cat 出现一次
#define Param_Error "Parameter Error.\n"//cat ls各出现一次
#define Ls_NoSuchDirectory "DIR NOT FOUND.\n"
#define Cat_NoSuchFile "FILE NOT FOUND\n"
//3.pre-set BPB area
int BytesPerSec; //每扇区字节数量 默认为512
int SecPerCluster; //每簇扇区数量 默认为1
int BytesPerCluster; //每簇字节数量
int ReservedSecCount; //为MBR保留的扇区数量 默认1
int NumOfFats; //多少个FAT表 默认2
int RootEntCnt; //根目录区文件最大数量 默认224
int FatSz16; //一个FAT表所占用的扇区 默认9
//4.pre-set address area
int fatBase; //fat表基址
int fileRootBase; //根目录基址
int dataBase; //数据基址
#pragma pack(1)
//5.namespace
using namespace std;
//6.announce print_func
extern "C" {void my_print(const char *, int);void change_to_red();void back_to_default();}
void myPrint(const char *s){my_print(s,strlen(s));}

//1.Node（称Tree也行）由于FAT12中文件以树状链表存放，我们也使用node数据结构存放img的树状结构。
class Node{//node里面多放点东西 根节点默认为一个目录节点，前驱为自己。
    string path;//路径
    string name;//目录或文件名称 path+name=all
    Node* pre{}; //前继节点
    vector<Node*> suc;//后继节点
    bool isFile = false;//文件或目录
    bool isVal = true;//值或非值 false则为.或..
    uint32_t size{};//如果是文件，则有大小。
    char *content = new char[10000];//如果是文件，则有存储。
    int directory_count = 0;//如果是目录，则有下级目录个数。
    int file_count = 0;//如果是目录，则有下级文件个数。
    public:
        Node() = default;//默认是目录，下级目录和文件为. ..
        Node(string name,bool isVal){this->name = std::move(name);this->isVal = isVal;}//一般非值
        Node(string name,string path){this->name = std::move(name);this->path = std::move(path);}//一般是目录
        Node(string name,uint32_t size,bool isFile,string path){this->name = std::move(name);this->path = std::move(path);this->isFile = isFile;this->size = size;}//一般是文件
        void setPath(string pat){this->path = std::move(pat);}
        void setName(string nam){this->name = std::move(nam);}
        void addChild(Node* su){this->suc.push_back(su);su->pre=this;}//只要加子节点，就保持树状双链表模式
        void addFileChild(Node* su){this->addChild(su);this->file_count=this->file_count+1;}
        void addDirChild(Node* su){this->addChild(su);this->directory_count=this->directory_count+1;su->addChild(new Node(".",false));su->addChild(new Node("..",false));}
        void setPre(Node* pr){this->pre=pr;}
        string getPath() {return this->path;}
        string getName() {return this->name;}
        Node* getPre() {return this->pre;}
        vector<Node*> getSuc() {return this->suc;}
        bool CheckIsFile() const {return this->isFile;}
        bool CheckIsVal() const {return this->isVal;}
        uint32_t getSize() const {return this->size;}
        char* getContent() {return this->content;}
        int get_dir_count() const {return this->directory_count;}
        int get_file_count() const {return this->file_count;}
};
//2.FAT12系统 存MBR？只需要存MBR里面的BPB部分即可。存FAT1/2？不需要，有地址就行。存根目录区？这个用得上。便于ls。存普通数据区？也不用，道理同FAT1/2。
//2.1由于不同img文件的BPB属性不同，需要提取和初始化。
class MBR{//完全按照引导扇区BPB的格式来
    uint16_t BPB_BytesPerSector{};//每扇区字节数 11-12（默认512） ref https://blog.csdn.net/qq_33316004/article/details/123277436
    uint8_t BPB_SectorsPerCluster{};//每簇扇区数 13-13（默认1）
    uint16_t BPB_ReservedSectorCount{};//MBR占用的扇区数 14-15（默认1）
    uint8_t BPB_NumFATs{};//FAT表的数量 16-16 （默认2）
    uint16_t BPB_RootEntryCount{};//根目录区文件最大数量 17-18 （默认224）
    __attribute__((unused)) uint16_t BPB_TotalSector16{};//限于16bit的扇区总数 19-20 （默认2880）
    __attribute__((unused)) uint8_t BPB_Media{};//介质描述符 21-21 （默认0xF0）
    uint16_t BPB_FATSector_z_16{};//一个FAT表所占扇区 22-23 （默认9）
    __attribute__((unused)) uint16_t BPB_SectorPerTrack{};//每磁道扇区数量 24-25 （无默认）
    __attribute__((unused)) uint16_t BPB_NumHeads{};//磁头数 26-27 （无默认）
    __attribute__((unused)) uint32_t BPB_HiddenSector{};//隐藏扇区数量 28-31 （默认0）
    __attribute__((unused)) uint32_t BPB_TotalSector32{};//16位存不下置0在这里存 32-35 （无默认）
    public:
        MBR() = default;
        void init(FILE* img){
            fseek(img,11,SEEK_SET);//重新定义文件指针
            fread(this,1,25,img);//以一个字节为单位基准，读取从第十一字节开始的25个字节
            //留下有用的
            BytesPerSec = this->BPB_BytesPerSector;
            SecPerCluster = this->BPB_SectorsPerCluster;
            BytesPerCluster = BytesPerSec*SecPerCluster;
            ReservedSecCount = this->BPB_ReservedSectorCount;
            NumOfFats = this->BPB_NumFATs;
            RootEntCnt = this->BPB_RootEntryCount;
            FatSz16 = this->BPB_FATSector_z_16;
            //基址
            fatBase = ReservedSecCount * BytesPerSec;
            fileRootBase = fatBase + NumOfFats * FatSz16 * BytesPerSec;
            dataBase = fileRootBase + (RootEntCnt * 32 + BytesPerSec -1) / BytesPerSec * BytesPerSec;//正确 因为簇是数据区的组织方式而非其他区域。
        }
};
//2.2我们需要根目录条目对Node进行填充。
class RootEntry{
    char fileName[8]{};//0-7
    char extendName[3]{};//8-10
    uint8_t attribute{};//11-11
    __attribute__((unused)) char reserved[10]{};//12-21
    __attribute__((unused)) uint16_t createTime{};//22-23
    __attribute__((unused)) uint16_t createDate{};//24-25
    uint16_t firstCluster{};//26-27
    uint32_t fileSize{};//28-31
    public:
        RootEntry() = default;//空文件
        bool isEmpty(){return this->fileName[0]=='\0';}//FAT12文件名称均为大写,非空不代表合法
        bool isInvalid(){
            int check = 0;
            for(char k : this->fileName){
                if(!((k>='A'&&k<='Z')||(k>='0'&&k<='9')||k==' ')){
                    check=1;
                }
            }
            for(char k : this->extendName){
                if(!((k>='A'&&k<='Z')||(k>='0'&&k<='9')||k==' ')){
                    check=1;
                }
            }
            return check;
        }//合法后不代表是文件
        bool isFile() const{return (this->attribute & 0x10 )== 0;}//初始化到根节点
        void init(FILE* img,Node* root){
            int base = fileRootBase; //base会发生变化
            char info[12];
            for(int i=0;i<RootEntCnt;i++){
                fseek(img,base,SEEK_SET);
                fread(this,1,32,img);
                base=base+32;
                if(!isEmpty()&&!isInvalid()){
                    if(isFile()){
                        fetchFileName(info);
                        Node* child = new Node(info,this->fileSize,true,root->getPath());//path+name
                        root->addFileChild(child);
                        getFileContent(img,this->firstCluster,child);//递归式
                    }else{
                        fetchDirName(info);
                        Node* child = new Node();
                        child->setName(info);
                        child->setPath(root->getPath());//path+name+/
                        root->addDirChild(child);
                        extendChild(img,this->firstCluster,child);//递归式
                    }
                }
            }
        }
        void fetchFileName(char info[12]){
            int tmp=0;
                int j=0;
                while(fileName[j]!=' '){
                    info[tmp++]=fileName[j]; j++;
                    if(j>=8) break;
                }
                info[tmp++]='.';
                j=0;
                while(extendName[j]!=' '){
                    info[tmp++]=extendName[j]; j++;
                    if(j>=3) break;
                }
                 info[tmp]='\0';
        }
        void fetchDirName(char info[12]){
            int tmp=0;
            for(char j : fileName){
                if(j!=' '){
                    info[tmp++]=j;
                }
            }
            info[tmp]='\0';
        }
        static void getFileContent(FILE* img,int Cluster,Node* root){
            if(Cluster == 0 || Cluster == 1){ return ;}
            int cluster = Cluster;
            int value = 0;
            char* tobe_filled = root->getContent();

            while(value < 0xFF8){//FF8h-FFFh 文件最后一个簇
                value = getValue(img,cluster);
                if(value == 0xFF7){
                    myPrint("broken cluster\n"); break;
                }
                char * str =(char*) malloc(BytesPerCluster);
                char * content = str;
                int start = dataBase + (cluster - 2) * BytesPerCluster;
                fseek(img,start,SEEK_SET);
                fread(content,1,BytesPerCluster,img);
                for(int i=0;i<BytesPerCluster;i++){
                    *tobe_filled = content[i];
                    tobe_filled++;
                }
                free(str);
                cluster = value;
            }
        }
        void extendChild(FILE* img,int Cluster,Node* root){
            if(Cluster == 0 || Cluster == 1){ return ;}
            int cluster = Cluster;
            int value = 0;

            while(value < 0xFF8){
                value = getValue(img,cluster);
                if(value == 0xFF7){
                    myPrint("broken cluster\n"); break;
                }
                int start = dataBase + (cluster-2) * BytesPerCluster;
                int loop = 0;
                while(loop < BytesPerCluster){
                    auto* rootEntry = new RootEntry();
                    fseek(img,start+loop,SEEK_SET);
                    fread(rootEntry,1,32,img);
                    loop=loop+32;

                    if (rootEntry->isEmpty() || rootEntry->isInvalid()) {
                        continue;
                    }
                    char tmpName[12];
                    if ((rootEntry->isFile())) {
                        rootEntry->fetchFileName(tmpName);
                        Node *child = new Node(tmpName, rootEntry->fileSize, true, root->getPath() + root->getName() + "/");
                        root->addFileChild(child);
                        getFileContent(img, rootEntry->firstCluster, child);
                    } else {
                        rootEntry->fetchDirName(tmpName);
                        Node *child = new Node();
                        child->setName(tmpName);
                        child->setPath(root->getPath() + root->getName() + "/" );
                        root->addDirChild(child);
                        extendChild(img, rootEntry->firstCluster, child);
                    }
                }
            }
        }
        static int getValue(FILE* img, int num){
            int base = ReservedSecCount * BytesPerSec;
            int pos = base + num * 3 / 2;//聚簇2 偏移3个Byte 聚簇3 偏移4个Byte(应该是4.5个) 处理方式不同
            uint16_t bytes;
            uint16_t *bytesPtr = &bytes;
            fseek(img,pos,SEEK_SET);
            fread(bytesPtr,1,2,img);
            if(num % 2 == 0){
                bytes = bytes << 4;
                return bytes >> 4;//不变
            }else{
                return bytes >> 4;
            }
        }
};
//3.输出
vector<string> path2unit(const string& path){
    char *s = new char[path.size()+1];
    strcpy(s,path.c_str());
    char *p = strtok(s, "/");
    vector<string> units;
    while(p){
        units.emplace_back(p);
        p=strtok(nullptr, "/");
    }
    return units;
}
Node* findObject(Node* root,const vector<string>& units){
    Node* tmp = root;
    for(auto & unit : units){
        if(unit == "."){
            continue;
        }else if(unit == ".."){
            tmp = tmp->getPre();
        }else{
            int notfound = 1;
            for(int j=0;j<tmp->getSuc().size();j++){
                if(tmp->getSuc()[j]->getName() == unit){
                    notfound=0; tmp=tmp->getSuc()[j];break;
                }
            }
            if(notfound){return nullptr;}
        }
    }
    return tmp;
}
void printCAT(Node* root,const string& path){
    vector<string> units = path2unit(path);
    Node* obj = findObject(root,units);
    if(obj == nullptr || !obj->CheckIsFile()){myPrint(Cat_NoSuchFile);return;}
    myPrint(obj->getContent());
}
void truePrintLS(Node* root){
    string path = root->getPath();
    string name = root->getName();
    path = path + name;
    if(path[path.size()-1] == '/'){path = path+ ":" + "\n";}else{path = path+ "/" + ":" + "\n";}
    myPrint(path.c_str());
    for(int i=0;i<root->getSuc().size();i++){
        Node* item = root->getSuc()[i];
        if(item->getName() == ".." || item->getName() == "." || !item->CheckIsFile()){
            change_to_red();
            myPrint(item->getName().c_str());
            back_to_default();
            myPrint("  ");
        }else{
            myPrint(item->getName().c_str());
            myPrint("  ");
        }
    }
    myPrint("\n");
    for(int i=0;i<root->getSuc().size();i++){
        Node* item = root->getSuc()[i];
        if(item->CheckIsVal() && !item->CheckIsFile()){
            truePrintLS(item);
        }
    }
}
void printLS(Node* root,const string& path){
    vector<string> units = path2unit(path);
    Node* obj = findObject(root,units);
    if(obj == nullptr || obj->CheckIsFile()){myPrint(Ls_NoSuchDirectory);return;}
    truePrintLS(obj);
}
void truePrintLSL(Node* root){
    string path = root->getPath();
    string name = root->getName();
    path = path + name;
    if(path[path.size()-1]!='/'){path = path + "/";}
    path = path + " " + to_string(root->get_dir_count()) + " " + to_string(root->get_file_count()) + ":\n";
    myPrint(path.c_str());
    for(int i=0;i<root->getSuc().size();i++){
        Node* item = root->getSuc()[i];
        if(item->getName() == ".." || item->getName() == "."){
            change_to_red();
            myPrint(item->getName().c_str());
            back_to_default();
            myPrint("\n");
        }else if(!item->CheckIsFile()){
            change_to_red();
            myPrint(item->getName().c_str());
            back_to_default();
            myPrint("  ");
            myPrint(to_string(item->get_dir_count()).c_str());
            myPrint(" ");
            myPrint(to_string(item->get_file_count()).c_str());
            myPrint("\n");
        }else{
            myPrint(item->getName().c_str());
            myPrint("  ");
            myPrint(to_string(item->getSize()).c_str());
            myPrint("\n");
        }
    }
    myPrint("\n");
    for(int i=0;i<root->getSuc().size();i++){
        Node* item = root->getSuc()[i];
        if(item->CheckIsVal() && !item->CheckIsFile()){
            truePrintLSL(item);
        }
    }
}
void printLSL(Node* root,const string& path){
    vector<string> units = path2unit(path);
    Node* obj = findObject(root,units);
    if(obj == nullptr || obj->CheckIsFile()){myPrint(Ls_NoSuchDirectory);return;}
    truePrintLSL(obj);
}
//4.主函数与输入处理
int main (){
    FILE* img;
    img = fopen("a.img","rb");

    Node* root = new Node();
    root->setPath("/");
    root->setName("");
    root->setPre(root);

    MBR* mbr = new MBR();
    mbr->init(img);

    auto* rootEntry = new RootEntry();
    rootEntry->init(img,root);

    while(true){
        myPrint(">");
        string str;
        getline(cin,str);//输入
        if(str.empty()){continue;}

        istringstream ss(str);
        vector<string> words;
        string word;
        while(ss >> word){words.push_back(word);}//处理输入

        if(words[0] == "exit"){break;}
        if(words[0] != "ls" && words[0] != "cat"){myPrint(Unknown_Command);}
        if(words[0] == "ls"){//ls 有三个参数：ls、路径和-l 多于一个路径参数、-l之后出现l之外的其它字母均为参数错误 正确但无文件夹归为找不到文件
            int check[words.size()];//1代表指令 2代表参数 3代表路径
            int error=0;
            for(int i=0;i<words.size();i++){
                if(words[i] == "ls"){
                    check[i] = 1; //1代表指令
                }else if(words[i][0] == '-'){
                    if(words[i].size() <= 1){error = 1;}
                    for(int j=1;j<words[i].length();j++){
                        if(words[i][j]!='l'){
                            error=1;
                        }
                    }
                    check[i] = 2; //2代表参数
                }else{
                    check[i] = 3; //3代表路径
                }
            }
            if(!error){
                int l_num=0;
                int path_num=0;
                for(int i=0;i<words.size();i++){
                    if(check[i] == 2){
                        l_num+=1;
                    }if(check[i] == 3){
                        path_num+=1;
                    }
                }
                if(path_num <=1){
                    int path_set = 0;
                    for(int i=0;i<words.size();i++){
                        if(check[i]==3){
                            path_set = i;
                        }
                    }
                    if(l_num&&path_set){
                        printLSL(root,words[path_set]);
                    }else if(l_num){
                        printLSL(root,"");
                    }else if(path_set){
                        printLS(root,words[path_set]);
                    }else{
                        printLS(root,"");
                    }
                }else{
                    myPrint(Param_Error);
                }
            }else{
                myPrint(Param_Error);
            }
        }
        if(words[0] == "cat"){//cat 只有两个参数：cat和路径 多于两个参数为参数错误 等于两个参数但无文件统一归为找不到文件
            if(words.size() == 2){
                printCAT(root,words[1]);
            }else{
                myPrint(Param_Error);
            }
        }
    }
    return 0;
}