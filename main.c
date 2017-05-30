#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int line=1;//全局变量 读到了第几行
FILE *C0_code;//全局变量，源代码文件变量
enum symbol{becames,add,minus,times,slash,less,lessorequal,more,moreorequal,notequal,equal,semicolon,constsym,intsym,ident,integer,comma
,floatsym,realnumber,charsym,character,unsignedinteger,leftbracket,rightbracket,leftparent,rightparent,leftcurlybracket,rightcurlybracket,
voidsym,mainsym,ifsym,elsesym,whilesym,switchsym,casesym,colon,scanfsym,printfsym,returnsym,string};
//枚举变量，所有出现的词的类型
enum symbol sym;//最近识别出来的符号类型
char *symbol_rem[]={"becames","add","minus","times","slash","less","lessorequal","more","moreorequal","notequal","equal","semicolon","constsym","intsym","ident","integer","comma"
,"floatsym","realnumber","charsym","character","unsignedinteger","leftbracket","rightbracket","leftparent","rightparent","leftcurlybracket","rightcurlybracket",
"voidsym","mainsym","ifsym","elsesym","whilesym","switchsym","casesym","colon","scanfsym","printfsym","returnsym","string"};
int integernum;//最近识别出来的整形
double realnum;//最近识别出来的实数
char id[20];//最近识别出来的标识符
char token[100];//词法分析临时缓冲区
char ch;//最近读入的字符
char zf;//最近读到的字符
char zfc[100];//最近读到的字符串
struct func{
    int num;//参数个数
    int parameter[10];//参数类型 0 int 1 float 2 char
    int big;//函数代码段开始部分
    int funcpoint;
};
union Valunion{
    int intval;
    char charval;
    double floatval;
    struct func funcval;
};
struct atab{
    char name[20];//标识符名字
	int lev;//局部变量还是全局变量,lev==0则为全局变量，否则为局部变量
	int type;//标识符类型,三大类型 int float char,分别为 0 1 2 ，3为void
	int degree;//数组长度
	int kind;//函数 变量 常量的区分 0 1 2
	union Valunion val;//具体值的联合
	int offset;//具体的地址偏移量
	int is_array;
}tab[500];//暂定符号表有500项
int tabtop;//符号表顶指针

char hc[50][100];//输出缓存区
int hc_top;//输出缓存区指针


enum p_code_name{ADD,LIT,SUB,MUL,DIV,LOD,LODT,LOA,STT,JPC,JMP,INT,STI,JMB,STO,QUT,LOG,LOGT,rei,ref,rec,wr,ws};//pcode代码操作符
char *P_code_name_name[]={"ADD","LIT","SUB","MUL","DIV","LOD","LODT","LOA","STT","JPC","JMP","INT","STI","JMB","STO","QUT","LOG","LOGT","rei","ref","rec","wr","ws"};
/*
    LIT 0 A,将立即数A存入栈顶
    ADD 0 0,栈顶与次栈顶相加，结果存于次栈顶
    SUB 0 0,次栈顶减栈顶，结果存于次栈顶
    MUL 0 0,次栈顶和栈顶乘，结果存于次栈顶
    DIV 0 0,次栈顶除以栈顶，结果存于次栈顶
    LOD l A,将层次为l，相对地址为A的变量置于栈顶
    LODT l 0,将地址为栈顶的变量取出置于栈顶,层次为l
    LOA l A,将层次为l,相对地址为A的地址取出置于栈顶
    STT l 0,将栈顶数存于次栈顶地址中,层次为l
    LOG 0 A,逻辑运算集合，进行次栈顶与栈顶的逻辑运算 结果存于次栈顶
            顺序依次为1 > 2 >= 3 < 4 <= 5 != 6 ==
    LOGT 0 A逻辑运算集合，进行次栈顶与栈顶的逻辑运算 结果存于栈顶
            顺序依次为1 > 2 >= 3 < 4 <= 5 != 6 ==
    JPC 0 A,若栈顶为0 则跳转到指令A
    JMP 0 A,无条件跳转到A
    INT 0 A,存储栈栈顶开辟A个存储单元
    STI l A,将立即数l存于相对地址A中
    JMB 0 0,跳至最近的ret addr处
    STO l A,将栈顶元素存于层次为l，相对地址为A的存储区
    QUT 0 0,存储栈退栈一次
    rei 0 0
    ref 0 0
    rec 0 0读取指令，从标准输入输出读取,存于栈顶
    wr 0 0标准输出指令，输出栈顶至标准输出
*/
union p_par{//目标码参数联合
    int zheng;
    float shi;
};
struct p_code_list{
    enum p_code_name id;
    union p_par par1;
    union p_par par2;
    int isfloat;
}p_code[500];//目标代码存储区
int p_top;//目标代码已写到函数

int main_big;//需要给main函数分配的空间大小
int main_entry;//程序入口
int main_tab;//main在tab中位置

int is_error=0;
int is_over=0;

void getch(){//读字符程序
    ch=fgetc(C0_code);
    //printf("%c\n",ch);
    //puts("1");
}
void error(int i){//报错函数  出现问题做相应的报错
    printf("something is wrong,error code%d,in line%d\n",i,line);
    switch(i){
    case 1:
        puts("词法错误,此处应为 =");
        break;
    case 2:
        puts("词法错误,0后面不允许直接接数字");
        break;
    case 3:
        puts("词法错误,标识符过长");
        break;
    case 4:
        puts("异常读入");
        break;
    case 5:
        puts("词法错误,缺少右单引号或右双引号");
        break;
    case 6:
        puts("语法错误,缺少类型标识符");
        break;
    case 7:
        puts("语义错误，常量定义重复");
        break;
    case 8:
        puts("语法错误，此处应为标识符");
        break;
    case 9:
        puts("语法错误，此处应为等号");
        break;
    case 10:
        puts("语义错误，常量定义类型不匹配");
        break;
    case 11:
        puts("语义错误，超越符号表上限");
        break;
    case 12:
        puts("语法错误，日常丢分号");
        break;
    case 13:
        puts("语法错误，缺少右方括号");
        break;
    case 14:
        puts("语法错误，此处应为无符号整数");
        break;
    case 15:
        puts("语法错误，不允许声明函数后再声明变量");
        break;
    case 16:
        puts("语法错误，此处应为类型标识符");
        break;
    case 17:
        puts("语法错误，此处应为右括号");
        break;
    case 18:
        puts("语法错误，此处应为左花括号");
        break;
    case 19:
        puts("语法错误，此处应为右花括号");
        break;
    case 20:
        puts("语法错误，标识符后只能接=或(");
        break;
    case 21:
        puts("语法错误，此处应为左括号");
        break;
    case 22:
        puts("语法错误，此处应为等号");
        break;
    case 23:
        puts("语法错误，此处应为可枚举变量");
        break;
    case 24:
        puts("语法错误，此处应为冒号");
        break;
    case 25:
        puts("语法错误，此处应为void");
        break;
    case 26:
        puts("语法错误，此处应为main");
        break;
    case 27:
        puts("语义错误，变量定义重复");
        break;
    case 28:
        puts("语义错误，标识符未在符号表中找到");
        break;
    case 29:
        puts("语义错误，数组访问越界");
        break;
    case 30:
        puts("语义错误，非变量不能引用数组");
        break;
    case 31:
        puts("语义错误，函数名没找到");
        break;
    case 32:
        puts("语义错误，参数类型不匹配");
        break;
    case 33:
        puts("语义错误，只允许往变量里读值");
        break;
    case 34:
        puts("语义错误，switch语句中的逻辑判断只能为可枚举变量，不能为实数");
        break;
    case 35:
        puts("语义错误，左右赋值类型不匹配");
        break;
    case 36:
        puts("语义错误，返回值类型不匹配");
        break;
    case 37:
        puts("语义错误，参数类型不匹配");
        break;
    case 38:
        puts("语法错误，奇怪的因子");
        break;
    case 39:
        puts("void函数类型不能返回值");
        break;
    }

    is_error=1;


}
void clear_token(int i){//清空临时token数组
    int ii;
    for(ii=0;ii<i;ii++){
        token[ii]=0;
    }
}
void getsym(){//词法分析程序
    if(is_over)return;
    int ii=0;
    int i=0;//token临时指针每到一个新词 i=0;
    while(ch==' '||ch=='\t'||ch=='\n'){//空字符跳过
        if(ch=='\n')line++;
        getch();

    }
    if(ch=='*'){sym=times;getch();}
    else if(ch=='/'){sym=slash;getch();}
    else if(ch==';'){sym=semicolon;getch();}
    else if(ch==':'){sym=colon;getch();}
    else if(ch=='+'){sym=add;getch();}
    else if(ch=='-'){sym=minus;getch();}
    else if(ch==','){sym=comma;getch();}
    else if(ch=='('){sym=leftparent;getch();}
    else if(ch==')'){sym=rightparent;getch();}
    else if(ch=='['){sym=leftbracket;getch();}
    else if(ch==']'){sym=rightbracket;getch();}
    else if(ch=='{'){sym=leftcurlybracket;getch();}
    else if(ch=='}'){sym=rightcurlybracket;getch();}
    else if(ch=='<'){
        getch();
        if(ch=='='){
            sym=lessorequal;
            getch();
        }
        else{
            sym=less;
        }
    }
    else if(ch=='>'){
        getch();
        if(ch=='='){
            sym=moreorequal;
            getch();
        }
        else{
            sym=more;
        }
    }
    else if(ch=='='){
        getch();
        if(ch=='='){
            sym=equal;
            getch();
        }
        else{
            sym=becames;
        }
    }
    else if(ch=='!'){
        getch();
        if(ch=='='){
            sym=notequal;
            getch();
        }
        else{
            error(1);
        }
    }
    else if(ch=='\''){
        getch();
        zf=ch;
        getch();
        if(ch!='\''){
            error(5);
        }
        else{
            getch();
        }
        sym=character;

    }
    else if(ch=='\"'){
        getch();
        while(ch==32||ch==33||(ch>=35&&ch<=126)){
            token[i]=ch;
            i++;
            getch();
        }
        if(ch!='\"'){
            error(5);
        }
        else{
            getch();
        }
        strcpy(zfc,token);
        clear_token(i);
        i=0;
        sym=string;
    }
    else if(ch>='1'&&ch<='9'){
        token[i]=ch;
        i++;
        getch();
        while(ch>='0'&&ch<='9'){
            token[i]=ch;
            i++;
            getch();
        }
        if(ch=='.'){
            token[i]=ch;
            i++;
            getch();
            while(ch>='0'&&ch<='9'){
                token[i]=ch;
                i++;
                getch();
            }
            realnum=atof(token);
            clear_token(i);
            i=0;
            sym=realnumber;
        }
        else{
            integernum=atoi(token);
            clear_token(i);
            i=0;
            sym=unsignedinteger;
        }
    }
    else if(ch=='0'){
        getch();
        if(ch>='0'&&ch<='9'){
            error(2);
        }
        else{
            if(ch=='.'){
                token[0]='0';
                token[1]='.';
                i=2;
                getch();
                while(ch>='0'&&ch<='9'){
                    token[i]=ch;
                    i++;
                    getch();
                }
                realnum=atof(token);
                clear_token(i);
                i=0;
                sym=realnumber;

            }
            else{
                sym=integer;
                integernum=0;
            }
        }
    }
    else if((ch>='A'&&ch<='Z')||ch=='_'||(ch>='a'&&ch<='z')){
        token[i]=ch;
        i++;
        getch();
        while((ch>='A'&&ch<='Z')||ch=='_'||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')){
            if(i>20){
                error(3);
            }
            token[i]=ch;
            i++;
            getch();
        }
        token[i]=0;
        if(strcmp(token,"const")==0){sym=constsym;}
        else if(strcmp(token,"int")==0){sym=intsym;}
        else if(strcmp(token,"float")==0){sym=floatsym;}
        else if(strcmp(token,"char")==0){sym=charsym;}
        else if(strcmp(token,"void")==0){sym=voidsym;}
        else if(strcmp(token,"main")==0){sym=mainsym;}
        else if(strcmp(token,"if")==0){sym=ifsym;}
        else if(strcmp(token,"else")==0){sym=elsesym;}
        else if(strcmp(token,"while")==0){sym=whilesym;}
        else if(strcmp(token,"switch")==0){sym=switchsym;}
        else if(strcmp(token,"case")==0){sym=casesym;}
        else if(strcmp(token,"scanf")==0){sym=scanfsym;}
        else if(strcmp(token,"printf")==0){sym=printfsym;}
        else if(strcmp(token,"return")==0){sym=returnsym;}
        else{
            sym=ident;
            ii=0;
            for(ii=0;ii<i;ii++){
                if(token[ii]>='A'&&token[ii]<='Z')
                token[ii]+=32;
            }
            strcpy(id,token);
        }
        clear_token(i);
        i=0;
    }
    else if(ch==EOF){
        puts("this file is end");
        is_over=1;
        return;
        //exit(1);
    }
    else{

        error(4);
        getch();
    }
    //printf("%d:~~~%s\n",line,symbol_rem[sym]);


}
void enter(int add,char *name,int lev,int type,int kind,int degree,int is_array){//加入符号表函数
    if(add>500){error(11);printf("符号太多了，爆符号表上限了，编译结束");exit(1);}
    strcpy(tab[add].name,name);
    tab[add].lev=lev;
    tab[add].type=type;
    tab[add].kind=kind;
    tab[add].degree=degree;
    tab[add].is_array=is_array;
    if(kind==2){
        if(add==1){tab[add].offset=3;}
        else{
            tab[add].offset=tab[add-1].offset;
        }
        switch(type){
        case 0:tab[add].val.intval=integernum;break;
        case 1:tab[add].val.floatval=realnum;break;
        case 2:tab[add].val.charval=zf;break;
        default:tab[add].val.intval=0;
    }
    }
    if(kind==1){
        if(add==1){tab[add].offset=3;}
        else{
            tab[add].offset=tab[add-1].offset+tab[add-1].degree;
        }

    }
    if(kind==0){
        tab[add].offset=3;
        tab[add].val.funcval.num=0;
    }

}
void gen(enum p_code_name name,int par1,int par2){
    p_code[p_top].id=name;
    p_code[p_top].par1.zheng=par1;
    p_code[p_top].par2.zheng=par2;
    p_code[p_top].isfloat=0;
    //printf("%s,%d,%d!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",P_code_name_name[p_code[p_top].id],p_code[p_top].par1.zheng,p_code[p_top].par2.zheng);
    p_top++;
}
void genf(enum p_code_name name,int par1,float par2){
    p_code[p_top].id=name;
    p_code[p_top].par1.zheng=par1;
    p_code[p_top].par2.shi=par2;
    p_code[p_top].isfloat=1;
    p_top++;
}
void enter_function_value(int type){
    int adr=tabtop;
    for(adr=tabtop;adr>0;adr--){
        if(tab[adr].kind==0)
            break;
    }
    tab[adr].val.funcval.num++;
    tab[adr].val.funcval.parameter[tab[adr].val.funcval.num-1]=type;
    tab[adr].val.funcval.funcpoint=p_top;
}
void enter_function_big(int big){
    int adr=tabtop;
    for(adr=tabtop;adr>0;adr--){
        if(tab[adr].kind==0)
            break;
    }
    tab[adr].val.funcval.big=big;
    tab[adr].val.funcval.funcpoint=p_top;

}
int find(char* name){//查找name是否跟符号表中某项重名，找到返回其位置，没有返回0
    int i=tabtop;
    for(i=tabtop;i>0;i--){
        if(tab[i].kind==0){
            return 0;//往前找到函数 直接结束
        }
        if(strcmp(name,tab[i].name)==0){
            return i;
        }
    }
    return 0;
}
int lookup(char *name){//查符号表函数，寻找名字为name的符号在符号表中的位置。
    int i=tabtop;
    for(i=tabtop;i>0;i--){//找局部变量
        if(tab[i].kind==0){break;}
        if(strcmp(name,tab[i].name)==0){
            return i;
        }
    }
    for(i=1;i<=tabtop;i++){
        if(tab[i].lev==1)continue;
        else{
            if(strcmp(name,tab[i].name)==0){
            return i;
            }
        }
    }
    return 0;
}
int lookupmain(char *name){//查符号表 只找全局变量
    int i=0;

    for(i=1;i<=tabtop;i++){
        if(tab[i].lev==1)continue;
        else{
            if(strcmp(name,tab[i].name)==0){
            return i;
            }
        }
    }
    return 0;
}
int factor(){//＜因子＞    ::= ＜标识符＞｜＜标识符＞‘[’＜表达式＞‘]’｜＜整数＞|＜实数＞|＜字符＞｜＜有返回值函数调用语句＞|‘(’＜表达式＞‘)’
    int i=0;
    int paradr=4;
    int tp=0;//类型标识，0，字符，1整形，2实数
    int ftp=0;
    if(sym==ident){
        i=lookup(id);
        if(i==0){
            error(28);
            gen(LIT,0,0);
            getsym();
            return 1;
        }
        getsym();
        if(tab[i].type==0){tp=1;}
        if(tab[i].type==1){tp=2;}
        if(sym==leftbracket){
            if(tab[i].kind!=1){
                error(30);
                gen(LIT,0,0);
                getsym();
                return 1;
            }
            getsym();

            expression();
            if(sym==rightbracket){
                getsym();
            }
            else{
                error(13);
            }
        }
        else if(sym==leftparent){
            if(tab[i].kind!=0){//判断是不是函数
                i=lookupmain(tab[i].name);
                if(i==0||tab[i].kind!=0){
                    error(31);
                    gen(LIT,0,0);
                    getsym();
                    return 1;
                }
            }

            getsym();
            if(sym==rightparent)//若无参数
            {   getsym();
                if(tab[i].val.funcval.num!=0){error(32);}
                gen(INT,i,tab[i].val.funcval.big);//分配存储栈
                gen(STI,p_top+2,2);
                gen(JMP,0,tab[i].val.funcval.funcpoint);

            }
            else{
                ftp=expression();
                if(ftp==2&&tab[i].val.funcval.parameter[0]==0||ftp==2&&tab[i].val.funcval.parameter[0]==2){
                    error(37);
                }
                paradr++;
                while(sym==comma){
                    getsym();
                    ftp=expression();
                    if(ftp==2&&tab[i].val.funcval.parameter[paradr-4]==0||ftp==2&&tab[i].val.funcval.parameter[paradr-4]==2){
                    error(37);
                    }
                    paradr++;
                    if(paradr-4>tab[i].val.funcval.num){error(32);}
                }
                if(sym==rightparent)getsym();
                else{
                    error(17);
                }
                gen(INT,i,tab[i].val.funcval.big);//分配存储栈
                for(paradr-=1;paradr>=4;paradr--){
                    gen(STO,1,paradr);
                }

                gen(STI,p_top+2,2);
                gen(JMP,0,tab[i].val.funcval.funcpoint);

            }
            gen(LOD,1,3);
            gen(QUT,0,0);
        }
        if(tab[i].kind==1&&tab[i].is_array==0){
            gen(LOD,tab[i].lev,tab[i].offset);
        }else if(tab[i].kind==1&&tab[i].is_array==1){
            gen(LIT,0,tab[i].offset);
            gen(ADD,0,0);
            gen(LODT,tab[i].lev,0);
        }
        else if(tab[i].kind==2){
            switch(tab[i].type){
                case 0:gen(LIT,0,tab[i].val.intval);break;
                case 1:genf(LIT,0,tab[i].val.floatval);break;
                case 2:gen(LIT,0,tab[i].val.charval);break;
            }
        }
    }
    else if(integerjudge()){gen(LIT,0,integernum);getsym();tp=1;}
    else if(realnumjudge()){
            genf(LIT,0,realnum);
            getsym();
            tp=2;
            }
    else if(sym==character){gen(LIT,0,zf);getsym();tp=0;}
    else if(sym==leftparent){
        getsym();
        tp=expression();
        if(sym==rightparent)getsym();
        else{error(17);}
    }
    else{
        error(38);
    }
    return tp;
}
int terms(){//＜项＞     ::= ＜因子＞{＜乘法运算符＞＜因子＞}
    int tp=0;
    int tpf=0;
    int mul_judge=0;
    tpf=factor();
    if(tp<tpf){
        tp=tpf;
    }
    while(sym==times||sym==slash){
        if(sym==times)mul_judge=1;
        else{mul_judge=0;}
        getsym();
        tpf=factor();
        if(tp<tpf){
        tp=tpf;
        }
        if(mul_judge)gen(MUL,0,0);
        else{gen(DIV,0,0);}
    }
    return tp;
}
int expression(){//＜表达式＞    ::= ［＋｜－］＜项＞{＜加法运算符＞＜项＞}
    int add_judge=1;
    int tp=0;
    int tpt=0;
    int genadd=0;
    if(sym==add||sym==minus){
        gen(LIT,0,0);
        if(sym==add){
            genadd=1;

        }
        else{
            genadd=2;

        }
        getsym();
    }
    tpt=terms();
    if(genadd!=0){
        if(genadd==1){
            gen(ADD,0,0);
        }
        else{
            gen(SUB,0,0);
        }
    }
    if(tp<tpt)tp=tpt;
    while(sym==add||sym==minus){
        if(sym==add)add_judge=1;
        else{add_judge=0;}
        getsym();
        tpt=terms();
        if(tp<tpt)tp=tpt;
        if(add_judge)gen(ADD,0,0);
        else{gen(SUB,0,0);}
    }
    //puts("This is a expression");
    return tpt;
}
void ifstatement(){//＜条件语句＞  ::=  if ‘(’＜条件＞‘)’＜语句＞［else＜语句＞］
    int logic_sy;
    int jmpoint1;
    int jmpoint2;
    int tp=0;
    getsym();
    if(sym!=leftparent){error(21);}
    getsym();
    expression();

    if(sym==more||sym==moreorequal||sym==less||sym==lessorequal||sym==notequal||sym==equal){
        if(sym==more)logic_sy=1;
        else if(sym==moreorequal)logic_sy=2;
        else if(sym==less)logic_sy=3;
        else if(sym==lessorequal)logic_sy=4;
        else if(sym==notequal)logic_sy=5;
        else if(sym==equal)logic_sy=6;
        getsym();
        expression();

        gen(LOG,0,logic_sy);
    }
    if(sym!=rightparent){error(17);}
    gen(JPC,0,0);
    jmpoint1=p_top-1;
    getsym();
    statement();
    p_code[jmpoint1].par2.zheng=p_top;
    if(sym==elsesym){
        gen(JMP,0,0);
        p_code[jmpoint1].par2.zheng=p_top;
        jmpoint2=p_top-1;
        getsym();
        statement();
        p_code[jmpoint2].par2.zheng=p_top;
    }
    //puts("This is a if statement");
}
void whilestatement(){//循环语句
    int logic_sy;
    int jmpoint1;
    int jmpoint2;
    int tp=0;
    jmpoint1=p_top;
    getsym();
    if(sym!=leftparent){error(21);}
    getsym();
    expression();

    if(sym==more||sym==moreorequal||sym==less||sym==lessorequal||sym==notequal||sym==equal){
        if(sym==more)logic_sy=1;
        else if(sym==moreorequal)logic_sy=2;
        else if(sym==less)logic_sy=3;
        else if(sym==lessorequal)logic_sy=4;
        else if(sym==notequal)logic_sy=5;
        else if(sym==equal)logic_sy=6;
        getsym();
        expression();

        gen(LOG,0,logic_sy);
    }
    if(sym!=rightparent){error(17);}
    getsym();
    gen(JPC,0,0);
    jmpoint2=p_top-1;
    statement();
    gen(JMP,0,jmpoint1);
    p_code[jmpoint2].par2.zheng=p_top;
    //puts("This is a while statement");
}
void callorassign(){//调用语句或者赋值语句
    int paradr=4;
    int i=0;
    int tpl;
    int tpr;
    int ftp=0;
    i=lookup(id);
    if(i==0){
        error(28);
        while(sym!=semicolon){
            getsym();
        }
        return;
    }
    tpl=tab[i].type;
    getsym();
    if(sym==leftparent){//call
        if(tab[i].kind!=0){
            i=lookupmain(tab[i].name);
            if(i==0||tab[i].kind!=0){
                error(31);
                while(sym!=semicolon){
                    getsym();
                }
                return;
            }
        }

        getsym();
        if(sym==rightparent){
                if(tab[i].val.funcval.num!=0){error(32);}
                gen(INT,i,tab[i].val.funcval.big);
                gen(STI,p_top+2,2);
                gen(JMP,0,tab[i].val.funcval.funcpoint);
                gen(QUT,0,0);
                getsym();
                //puts("this is a call statement");return;

        }
        else{
            ftp=expression();
            if(ftp==2&&tab[i].val.funcval.parameter[paradr-4]==0||ftp==2&&tab[i].val.funcval.parameter[paradr-4]==2){
                error(37);
            }
            //gen(STO,1,paradr);
            paradr++;
            if(paradr-4>tab[i].val.funcval.num){error(32);}
            while(sym==comma){
                getsym();
                ftp=expression();
                if(ftp==2&&tab[i].val.funcval.parameter[paradr-4]==0||ftp==2&&tab[i].val.funcval.parameter[paradr-4]==2){
                    error(37);
                }
                //gen(STO,1,paradr);
                paradr++;
                if(paradr-4>tab[i].val.funcval.num){error(32);}
            }
            if(sym==rightparent){
                getsym();
            }
            else{
                error(17);
            }
            gen(INT,i,tab[i].val.funcval.big);
            for(paradr-=1;paradr>=4;paradr--){
                    gen(STO,1,paradr);
                }
            gen(STI,p_top+2,2);
            gen(JMP,0,tab[i].val.funcval.funcpoint);
            gen(QUT,0,0);
        }
        //puts("this is a call statement");
        return;
    }
    if(sym==leftbracket){
        getsym();

        expression();
        if(sym==rightbracket){
            getsym();
        }
        else{error(13);}
    }
    if(tab[i].is_array){
        gen(LOA,tab[i].lev,tab[i].offset);
        gen(ADD,0,0);
    }
    else{
        gen(LOA,tab[i].lev,tab[i].offset);
    }
    if(sym==becames){
        getsym();
    }
    else{
        error(22);
        while(sym!=semicolon){
            getsym();
        }
        return;
    }
    tpr=expression();
    if(tpl==0&&tpr==2||tpr==2&&tpl==2)error(35);
    if(tpl==1&&tpr==0||tpl==1&&tpr==1){
        gen(STT,0,1);
    }
    else{
        gen(STT,0,0);
    }

    //puts("this is a assign statement");
}
void readstatement(){//读语句
    getsym();
    int i=0;
    if(sym!=leftparent){
            error(21);
            while(sym!=semicolon){
                getsym();
            }
            return;
    }

    else{
        getsym();
    }
    if(sym==ident){

        i=lookup(id);
        if(i==0){
        error(28);
        while(sym!=semicolon){
            getsym();
        }
        return;
        }
        if(tab[i].kind!=1){
            error(33);
            while(sym!=semicolon){
                getsym();
            }
            return;
        }

        if(tab[i].type==0){
            gen(rei,0,0);
        }
        else if(tab[i].type==1){
            gen(ref,0,0);
        }
        else if(tab[i].type==2){
            gen(rec,0,0);
        }
        gen(STO,tab[i].lev,tab[i].offset);
        getsym();
    }
    else{
        error(8);
    }
    while(sym==comma){
        getsym();
        if(sym==ident){
            i=lookup(id);
            if(i==0){
            error(28);
            while(sym!=semicolon){
                getsym();
            }
            return;
            }
            if(tab[i].kind!=1){
                error(33);
                while(sym!=semicolon){
                    getsym();
                }
                return;
            }
            if(tab[i].type==0){
                gen(rei,0,0);
            }
            else if(tab[i].type==1){
                gen(ref,0,0);
            }
            else if(tab[i].type==2){
                gen(rec,0,0);
            }
            gen(STO,tab[i].lev,tab[i].offset);
            getsym();
        }
        else{
            error(8);
        }
    }
    if(sym==rightparent)getsym();
    else{error(17);}
    //puts("this is a read statement");
}
void writestatement(){//写语句
    int tp;
    getsym();
    if(sym==leftparent){getsym();}
    else{error(21);}
    if(sym==string){
        strcpy(hc[hc_top],zfc);
        gen(ws,0,hc_top);
        hc_top++;

        getsym();
        if(sym==comma){
            getsym();
            tp=expression();
            gen(wr,0,tp);
        }
    }
    else{
        tp=expression();
        gen(wr,0,tp);
    }
    if(sym==rightparent)getsym();
    else{error(17);}
    //puts("this is a write statement");

}
void switchstatement(){//switch语句
    int jmpoint=0;
    int jmpoint2[999];
    int jpd=0;
    int tp=0;
    int i;
    getsym();
    if(sym==leftparent)getsym();
    else{error(21);}
    tp=expression();
    if(tp>1)error(34);
    if(sym==rightparent)getsym();
    else{error(17);}

    if(sym==leftcurlybracket)getsym();
    else{error(18);}
    while(sym==casesym){
        getsym();
        if(integerjudge()){
            gen(LIT,0,integernum);
            gen(LOGT,0,6);
            gen(JPC,0,0);
            jmpoint=p_top;
            getsym();
        }
        else if(sym==character){
            gen(LIT,0,zf);
            gen(LOGT,0,6);
            gen(JPC,0,0);
            jmpoint=p_top;
            getsym();
        }
        else{error(23);}
        if(sym==colon)getsym();
        else{error(24);}
        statement();
        gen(JMP,0,0);
        jmpoint2[jpd]=p_top-1;
        jpd++;
        p_code[jmpoint-1].par2.zheng=p_top;
    }
    if(sym==rightcurlybracket)getsym();
    else{error(19);}
    for(i=0;i<jpd;i++){
        p_code[jmpoint2[i]].par2.zheng=p_top;
    }

    //puts("this is a switch statement");
}
void returnstatement(){//返回语句
    getsym();
    int i;
    int isvoid=0;
    i=tabtop;
    for(i=tabtop;i>0;i--){
        if(tab[i].kind==0){
            if(tab[i].type==3){
                isvoid=1;
            }
            break;
        }
    }
    if(sym==leftparent){
            if(isvoid){
                error(39);
            }
        getsym();
        expression();
        gen(STO,1,3);
        if(sym==rightparent)getsym();
        else{error(17);}
        gen(JMB,0,0);


    }
    else{
       gen(JMB,0,0);
    }
    //puts("this is a return statement");
}

void statement(){//语句



        switch(sym){//各种可能的语句 要求语句务必最后读下一个sym
            case ifsym:ifstatement();break;
            case whilesym:whilestatement();break;
            case leftcurlybracket:{
                getsym();
                while(sym!=rightcurlybracket){

                    statement();

                }
                getsym();
                break;
            }
            case ident:{
                callorassign();
                if(sym==semicolon){getsym();}
                else{error(12);}
                break;
            }
            case scanfsym:
                readstatement();
                if(sym==semicolon){getsym();}
                else{error(12);}
                break;
            case printfsym:
                writestatement();
                if(sym==semicolon){getsym();}
                else{error(12);}
                break;
            case semicolon:getsym();break;
            case switchsym:switchstatement();break;
            case returnsym:
                returnstatement();
                if(sym==semicolon){getsym();}
                else{error(12);}
                break;


    }
}
int vardefine(){//正儿八经的变量说明递归下降子程序
    int v_type=-1;//临时 标识符的类型,初始为未识别类型
    int v_degree=1;//临时，初始长度
    int v_add;//将要操作的符号表项
    int v_array=0;
    int v_big=0;
    while(sym==intsym||sym==floatsym||sym==charsym){
        switch(sym){//记录常量类型

        case intsym:
            v_type=0;
            break;
        case floatsym:
            v_type=1;
            break;
        case charsym:
            v_type=2;
            break;
        default:
            v_type=-1;
            error(6);
        }
        getsym();
        if(sym!=ident){
            error(8);
        }
        else{
            if(v_add=find(id)){
                error(27);//常量重复定义

            }
            if(v_add==0){
                tabtop++;
                v_add=tabtop;
            }
        }
        getsym();
        if(sym==leftbracket){//数组已识别
            v_array=1;
            getsym();
            if(sym==unsignedinteger){
                v_degree=integernum;
                getsym();
                if(sym==rightbracket){
                    getsym();
                }
                else{
                    error(13);
                }
            }
            else{
                error(14);
            }
        }
        enter(v_add,id,1,v_type,1,v_degree,v_array);

        v_degree=1;
        v_array=0;
        while(sym==comma){
            getsym();
            if(sym!=ident){
                error(8);
            }
            else{
                if(v_add=find(id)){
                    error(27);//常量重复定义

                }
                if(v_add==0){
                    tabtop++;
                    v_add=tabtop;
                }
            }
            getsym();
            if(sym==leftbracket){//数组已识别
                v_array=1;
                getsym();
                if(sym==unsignedinteger){
                    v_degree=integernum;
                    getsym();
                    if(sym==rightbracket){
                        getsym();
                    }
                    else{
                        error(13);
                    }
                }
                else{
                    error(14);
                }
            }
            enter(v_add,id,1,v_type,1,v_degree,v_array);

            v_degree=1;
            v_array=0;
        }
        if(sym!=semicolon){error(12);}
        else{
            getsym();
        }
    //puts("this is a var define in a function");
    }
    v_big=tab[tabtop].offset+v_degree-1;
    return v_big;
}
void vardefine_inmain(){//变量说明递归下降子程序，在主函数中调用，同时囊括函数定义
    int var_over=0;//标记变量，当其为1时，表明变量定义已经完成，进入函数定义阶段
    int v_type=-1;
    int v_degree=1;
    int v_add;
    int v_array=0;
    int v_big=0;
    int f_big=0;
    while(sym==intsym||sym==floatsym||sym==charsym||sym==voidsym){
        if(sym==voidsym){var_over=1;}//一旦读到void那么变量定义已经结束了
        switch(sym){
        case intsym:
            v_type=0;
            break;
        case floatsym:
            v_type=1;
            break;
        case charsym:
            v_type=2;
            break;
        case voidsym:
            v_type=3;
            break;
        default:
            v_type=-1;
        }
        getsym();
        if(sym==mainsym){
            gen(INT,-1,v_big);
            return;
        }
        if(sym!=ident){
            error(8);
        }
        else{
            if(v_add=find(id)){

                error(27);
            }
            else{
                tabtop++;
                v_add=tabtop;
            }
        }
        getsym();
        if(sym==leftparent){//发现左括号 进入函数阶段
            enter(v_add,id,0,v_type,0,1,0);
            var_over=1;
            getsym();
            if(sym==intsym||sym==floatsym||sym==charsym){
                switch(sym){
                    case intsym:
                        v_type=0;
                        break;
                    case floatsym:
                        v_type=1;
                        break;
                    case charsym:
                        v_type=2;
                        break;
                    default:
                        v_type=-1;
                    }
                getsym();
                if(sym!=ident){
                    error(8);
                }
                else{
                   if(v_add=find(id)){

                        error(27);//变量重复定义
                    }
                    if(v_add==0){
                        tabtop++;
                        v_add=tabtop;
                    }
                }

                enter(v_add,id,1,v_type,1,1,0);
                enter_function_value(v_type);
                getsym();
                while(sym==comma){
                    getsym();
                    if(sym==intsym||sym==floatsym||sym==charsym){
                        switch(sym){
                        case intsym:
                            v_type=0;
                            break;
                        case floatsym:
                            v_type=1;
                            break;
                        case charsym:
                            v_type=2;
                            break;
                        default:
                            v_type=-1;
                        }
                        getsym();
                        if(sym!=ident){
                            error(8);
                        }
                        else{
                            if(v_add=find(id)){
                                error(27);
                            }
                            else{
                                tabtop++;
                                v_add=tabtop;
                            }
                        }

                        enter(v_add,id,1,v_type,1,1,0);
                        enter_function_value(v_type);
                        getsym();
                    }
                    else{
                        error(16);
                    }
                }
            }
            if(sym!=rightparent){
                error(17);
            }
            getsym();
            if(sym!=leftcurlybracket){
                error(18);
            }
            getsym();
            constdefine(1);
            f_big=vardefine();
            enter_function_big(f_big);

            while(sym!=rightcurlybracket){

                    statement();

            }
            gen(JMB,0,0);


            getsym();
            //puts("this is a function define in main");
            continue;
        }
        if(sym==leftbracket&&var_over==0){//数组已识别
            v_array=1;
            getsym();
            if(sym==unsignedinteger){
                v_degree=integernum;
                getsym();
                if(sym==rightbracket){
                    getsym();
                }
                else{
                    error(13);
                }
            }
            else{
                error(14);
            }
        }
        else if(var_over==1&&sym==leftbracket){//函数不能为数组
            error(15);
        }
        enter(v_add,id,0,v_type,1,v_degree,v_array);
        v_big=tab[tabtop].offset+v_degree;
        v_degree=1;
        v_array=0;
        while(sym==comma&&var_over==0){
            getsym();
            if(sym!=ident){
                error(8);
            }
            else{
            if(v_add=find(id)){
                error(7);
            }
            else{
                tabtop++;
                v_add=tabtop;
            }
            }
            getsym();
            if(sym==leftbracket){//数组已识别
                v_array=1;
                getsym();
                if(sym==unsignedinteger){
                    v_degree=integernum;
                    getsym();
                    if(sym==rightbracket){
                        getsym();
                    }
                    else{
                        error(13);
                    }
                }
                else{
                    error(14);
                }
            }
            enter(v_add,id,0,v_type,1,v_degree,v_array);
            v_big=tab[tabtop].offset+v_degree;
            v_degree=1;
            v_array=0;
        }
        if(sym==semicolon&&var_over==0){getsym();}//puts("this is a var define in main");
        else{
            if(sym!=semicolon&&var_over==0){error(12);}
        }
        //enter(v_add,id,0,v_type,1,v_degree);语法分析 暂不入符号表
    }
    gen(INT,-1,v_big);
}
int integerjudge(){//是不是整形的判断
    int minus_judge=0;
    if(sym==integer)return 1;
    else if(sym==add||sym==minus){
        if(sym==minus)minus_judge=1;
        getsym();
        if(sym==unsignedinteger){if(minus_judge)integernum=-integernum;return 1;}
    }
    else if(sym==unsignedinteger)return 1;
    else{return 0;}
}
int realnumjudge(){//实数判断
    int minus_judge=0;
    if(sym==realnumber)return 1;
    else if(sym==add||sym==minus){
        if(sym==minus)minus_judge=1;
        getsym();
        if(sym==realnumber){if(minus_judge)realnum=-realnum;return 1;}
    }
    else return 0;
}
void constdefine(int c_lev){//常量说明递归下降子程序
    int c_type=-1;//临时 标识符的类型,初始为未识别类型
    int c_add;//将要操作的符号表项
    while(sym==constsym){
        getsym();
        switch(sym){//记录常量类型
        case intsym:
            c_type=0;
            break;
        case floatsym:
            c_type=1;
            break;
        case charsym:
            c_type=2;
            break;
        default:
            c_type=-1;
            error(6);
        }
        getsym();
        if(sym==ident){
            if(c_add=find(id)){
                error(7);//常量重复定义
            }
            if(c_add==0){
                tabtop++;
                c_add=tabtop;
            }
        }
        else{error(8);}
        getsym();
        if(sym!=becames){
            error(9);
        }
        getsym();
        if(integerjudge()&&(c_type==0||c_type==2)){

            getsym();
        }
        else if(realnumjudge()&&c_type==1){

            getsym();
        }
        else if(sym==character&&(c_type==0||c_type==2)){

            getsym();
        }
        else{
            error(10);//前后类型一致性判定
        }
        enter(c_add,id,c_lev,c_type,2,1,0);


        while(sym==comma){
            getsym();
            if(sym==ident){
                if(c_add=find(id)){
                    error(7);//常量重复定义
                }
                if(c_add==0){
                    tabtop++;
                    c_add=tabtop;
                }
            }
            else{error(8);}
            getsym();
            if(sym!=becames){
                error(9);
            }
            getsym();
            if(integerjudge()&&c_type==0){

            getsym();
            }
            else if(realnumjudge()&&c_type==1){

                getsym();
            }
            else if(sym==character&&c_type==2){

                getsym();
            }
            else{
                error(10);//前后类型一致性判定
            }
            enter(c_add,id,c_lev,c_type,2,1,0);


        }
        if(sym!=semicolon){
            error(12);
        }
        //puts("this is a constdefine in main");
        getsym();
    }
}
void mainfun(){//主函数

    if(sym==mainsym)getsym();
    else{error(26);}
    if(sym==leftparent)getsym();
    else{error(21);}
    if(sym==rightparent)getsym();
    else{error(17);}
    if(sym==leftcurlybracket)getsym();
    else{error(18);}
    tabtop++;
    enter(tabtop,"MAIN",0,0,0,1,0);
    main_tab=tabtop;
    constdefine(1);
    main_big=vardefine();
    main_entry=p_top;

    while(sym!=rightcurlybracket){

            statement();

    }
    getsym();

    //puts("this's main");
}
void program(){//递归调用子程序之程序
    int mainpoint1;
    int mainpoint2;
    int mainpoint3;
    int startpoint;
    getch();
    getsym();//获取第一个词
    mainpoint2=p_top;
    gen(JMP,0,0);
    constdefine(0);
    vardefine_inmain();
    startpoint=p_top-1;
    mainpoint1=p_top;
    gen(INT,1,0);
    gen(STI,999999,2);
    mainpoint3=p_top;

    gen(JMP,0,0);

    mainfun();
    p_code[mainpoint1].par2.zheng=main_big;
    p_code[mainpoint1].par1.zheng=main_tab;
    p_code[mainpoint2].par2.zheng=startpoint;
    p_code[mainpoint3].par2.zheng=main_entry;

}
void interupt(){//解释执行函数
    //存储区：
    //代码区 p_code
    enum p_code_name p;//指令寄存器
    int pc=0;//指令地址寄存器
    //存储栈

    int mbase=0;//基地址寄存器
    int mtop=0;//存储栈栈顶寄存器
    //运行栈
    struct arun{
        int isfloat;
        int zheng;
        double shi;
    }run[500],mem[500];
    int rtop=0;//运行栈栈顶寄存器

    while(pc<p_top){//在程序没有结束的情况下
        //printf("%d\n",pc);
        p=p_code[pc].id;//获取操作码
        int l;
        int A;
        switch(p){
        case ADD:
            if((!run[rtop-1].isfloat)&&(!run[rtop].isfloat)){run[rtop-1].zheng=run[rtop-1].zheng+run[rtop].zheng;}
            else if((run[rtop-1].isfloat)&&(!run[rtop].isfloat)){run[rtop-1].shi=run[rtop-1].shi+(double)run[rtop].zheng;}
            else if((!run[rtop-1].isfloat)&&(run[rtop].isfloat)){run[rtop-1].shi=(double)run[rtop-1].zheng+run[rtop].shi;run[rtop-1].isfloat=1;}
            else if((run[rtop-1].isfloat)&&(run[rtop].isfloat)){run[rtop-1].shi=run[rtop-1].shi+run[rtop].shi;}
            rtop--;
            pc++;
            break;

        case SUB:
            if((!run[rtop-1].isfloat)&&(!run[rtop].isfloat)){run[rtop-1].zheng=run[rtop-1].zheng-run[rtop].zheng;}
            else if((run[rtop-1].isfloat)&&(!run[rtop].isfloat)){run[rtop-1].shi=run[rtop-1].shi-(double)run[rtop].zheng;}
            else if((!run[rtop-1].isfloat)&&(run[rtop].isfloat)){run[rtop-1].shi=(double)run[rtop-1].zheng-run[rtop].shi;run[rtop-1].isfloat=1;}
            else if((run[rtop-1].isfloat)&&(run[rtop].isfloat)){run[rtop-1].shi=run[rtop-1].shi-run[rtop].shi;}
            rtop--;
            pc++;
            break;

        case MUL:
            if((!run[rtop-1].isfloat)&&(!run[rtop].isfloat)){run[rtop-1].zheng=run[rtop-1].zheng*run[rtop].zheng;}
            else if((run[rtop-1].isfloat)&&(!run[rtop].isfloat)){run[rtop-1].shi=run[rtop-1].shi*(double)run[rtop].zheng;}
            else if((!run[rtop-1].isfloat)&&(run[rtop].isfloat)){run[rtop-1].shi=(double)run[rtop-1].zheng*run[rtop].shi;run[rtop-1].isfloat=1;}
            else if((run[rtop-1].isfloat)&&(run[rtop].isfloat)){run[rtop-1].shi=run[rtop-1].shi*run[rtop].shi;}
            rtop--;
            pc++;
            break;

        case DIV:
            if((!run[rtop-1].isfloat)&&(!run[rtop].isfloat)){
                    run[rtop-1].zheng=run[rtop-1].zheng/run[rtop].zheng;
            }
            else if((run[rtop-1].isfloat)&&(!run[rtop].isfloat)){
                run[rtop-1].shi=run[rtop-1].shi/(double)run[rtop].zheng;
                }
            else if((!run[rtop-1].isfloat)&&(run[rtop].isfloat)){
                run[rtop-1].shi=(double)run[rtop-1].zheng/run[rtop].shi;run[rtop-1].isfloat=1;
                }
            else if((run[rtop-1].isfloat)&&(run[rtop].isfloat)){
                run[rtop-1].shi=run[rtop-1].shi/run[rtop].shi;
                }
            rtop--;
            pc++;
            break;

        // LIT 0 A,将立即数A存入栈顶
        case LIT:
            rtop++;
            if(p_code[pc].isfloat){
                run[rtop].isfloat=1;
                run[rtop].shi=p_code[pc].par2.shi;
            }
            else if(!p_code[pc].isfloat){
                run[rtop].isfloat=0;
                run[rtop].zheng=p_code[pc].par2.zheng;
            }
            pc++;
            break;
        // LOD l A,将层次为l，相对地址为A的变量置于栈顶
        case LOD:
            rtop++;
            l=p_code[pc].par1.zheng;
            A=p_code[pc].par2.zheng;
            if(l==0){
                if(mem[A].isfloat){
                    run[rtop].isfloat=1;
                    run[rtop].shi=mem[A].shi;
                }
                else{
                    run[rtop].isfloat=0;
                    run[rtop].zheng=mem[A].zheng;
                }
            }
            else{
                if(mem[A+mbase].isfloat){
                    run[rtop].isfloat=1;
                    run[rtop].shi=mem[A+mbase].shi;
                }
                else{
                    run[rtop].isfloat=0;
                    run[rtop].zheng=mem[A+mbase].zheng;
                }
            }
            pc++;
            break;
        //LODT l 0,将地址为栈顶的变量取出置于栈顶,层次为l
        case LODT:
            l=p_code[pc].par1.zheng;

            if(l==0){
                if(mem[run[rtop].zheng].isfloat){
                    run[rtop].isfloat=1;
                    run[rtop].shi=mem[run[rtop].zheng].shi;
                }
                else{
                    run[rtop].isfloat=0;
                    run[rtop].zheng=mem[run[rtop].zheng].zheng;
                }
            }
            else{
                if(mem[run[rtop].zheng+mbase].isfloat){
                    run[rtop].isfloat=1;
                    run[rtop].shi=mem[run[rtop].zheng+mbase].shi;
                }
                else{
                    run[rtop].isfloat=0;
                    run[rtop].zheng=mem[run[rtop].zheng+mbase].zheng;
                }
            }
            pc++;
            break;
        //LOA l A,将层次为l,相对地址为A的地址取出置于栈顶
        case LOA:
            rtop++;
            l=p_code[pc].par1.zheng;
            A=p_code[pc].par2.zheng;
            if(l==0){
                run[rtop].isfloat=0;
                run[rtop].zheng=A;
            }
            else{
                run[rtop].isfloat=0;
                run[rtop].zheng=A+mbase;
            }
            pc++;
            break;
         //STT l 0,将栈顶数存于次栈顶地址中
        case STT:


        if(p_code[pc].par2.zheng==1){
            if(run[rtop].isfloat){
            mem[run[rtop-1].zheng].isfloat=1;
            mem[run[rtop-1].zheng].shi=run[rtop].shi;
            }
            else{
            mem[run[rtop-1].zheng].isfloat=1;
            mem[run[rtop-1].zheng].shi=run[rtop].zheng;
        }
        }
        else{
            if(run[rtop].isfloat){
            mem[run[rtop-1].zheng].isfloat=1;
            mem[run[rtop-1].zheng].shi=run[rtop].shi;
            }
            else{
            mem[run[rtop-1].zheng].isfloat=0;
            mem[run[rtop-1].zheng].zheng=run[rtop].zheng;
        }
        }



            rtop-=2;
            pc++;
            break;
        //LOG 0 A,逻辑运算集合，进行次栈顶与栈顶的逻辑运算 结果存于次栈顶
        //顺序依次为1 > 2 >= 3 < 4 <= 5 != 6 ==
        case LOG:

            switch(p_code[pc].par2.zheng){
            case 1:
                if(run[rtop].isfloat==0&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng>run[rtop].zheng;
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng>run[rtop].shi;
                else if(run[rtop].isfloat==0&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi>run[rtop].zheng;
                }
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi>run[rtop].shi;
                }
                break;
            case 2:
                if(run[rtop].isfloat==0&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng>=run[rtop].zheng;
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng>=run[rtop].shi;
                else if(run[rtop].isfloat==0&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi>=run[rtop].zheng;
                }
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi>=run[rtop].shi;
                }
                break;
            case 3:
                if(run[rtop].isfloat==0&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng<run[rtop].zheng;
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng<run[rtop].shi;
                else if(run[rtop].isfloat==0&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi<run[rtop].zheng;
                }
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi<run[rtop].shi;
                }
                break;
            case 4:
                if(run[rtop].isfloat==0&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng<=run[rtop].zheng;
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng<=run[rtop].shi;
                else if(run[rtop].isfloat==0&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi<=run[rtop].zheng;
                }
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi<=run[rtop].shi;
                }
                break;
            case 5:
                if(run[rtop].isfloat==0&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng!=run[rtop].zheng;
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng!=run[rtop].shi;
                else if(run[rtop].isfloat==0&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi!=run[rtop].zheng;
                }
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi!=run[rtop].shi;
                }
                break;
            case 6:
                if(run[rtop].isfloat==0&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng==run[rtop].zheng;
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==0)run[rtop-1].zheng=run[rtop-1].zheng==run[rtop].shi;
                else if(run[rtop].isfloat==0&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi==run[rtop].zheng;
                }
                else if(run[rtop].isfloat==1&&run[rtop-1].isfloat==1){
                        run[rtop-1].isfloat=0;
                        run[rtop-1].zheng=run[rtop-1].shi==run[rtop].shi;
                }
                break;
            }
            rtop--;
            pc++;
            break;
        //LOGT 0 A逻辑运算集合，进行次栈顶与栈顶的逻辑运算 结果存于栈顶
        //顺序依次为1 > 2 >= 3 < 4 <= 5 != 6 ==
        case LOGT:
            if(run[rtop].isfloat||run[rtop-1].isfloat)error(34);
            switch(p_code[pc].par2.zheng){
            case 1:
                run[rtop].zheng=run[rtop-1].zheng>run[rtop].zheng;
                break;
            case 2:
                run[rtop].zheng=run[rtop-1].zheng>=run[rtop].zheng;
                break;
            case 3:
                run[rtop].zheng=run[rtop-1].zheng<run[rtop].zheng;
                break;
            case 4:
                run[rtop].zheng=run[rtop-1].zheng<=run[rtop].zheng;
                break;
            case 5:
                run[rtop].zheng=run[rtop-1].zheng!=run[rtop].zheng;
                break;
            case 6:
                run[rtop].zheng=run[rtop-1].zheng==run[rtop].zheng;
                break;
            }
            pc++;
            break;
        //JPC 0 A,若栈顶为0 则跳转到指令A
        case JPC:
             if(run[rtop].isfloat){
                if(run[rtop].shi==0){
                    pc=p_code[pc].par2.zheng;
                }
                else{
                    pc++;
                }
             }
             else{
                if(run[rtop].zheng==0){
                    pc=p_code[pc].par2.zheng;
                }
                else{
                    pc++;
                }
             }

             rtop--;
             break;
        //JMP 0 A,无条件跳转到A
        case JMP:
            pc=p_code[pc].par2.zheng;
            break;
        //INT i A,存储栈栈顶开辟A个存储单元，这些存储单元是为符号表中i函数准备的
        case INT:

            mem[mtop+1].isfloat=0;
            mem[mtop+1].zheng=mbase;
            mbase=mtop;
            mtop=mtop+p_code[pc].par2.zheng;
            mem[mbase+2].isfloat=0;
            if(tab[p_code[pc].par1.zheng].type==1){mem[mbase+3].isfloat=1;}
            else{mem[mbase+3].isfloat=0;}
            pc++;
            break;
        //STI l A,将立即数l存于相对地址A中
        case STI:
            mem[mbase+p_code[pc].par2.zheng].isfloat=0;
            mem[mbase+p_code[pc].par2.zheng].zheng=p_code[pc].par1.zheng;
            pc++;
            break;
        //JMB 0 0,跳至最近的ret addr处
        case JMB:
            pc=mem[mbase+2].zheng;
            break;
        //STO l A,将栈顶元素存于层次为l，相对地址为A的存储区
        case STO:
            l=p_code[pc].par1.zheng;
            A=p_code[pc].par2.zheng;
            if(A!=3){
            if(l==0){
                if(run[rtop].isfloat){
                    mem[A].isfloat=1;
                    mem[A].shi=run[rtop].shi;
                }
                else{
                    mem[A].isfloat=0;
                    mem[A].zheng=run[rtop].zheng;
                }
            }
            else{
                if(run[rtop].isfloat){
                    mem[A+mbase].isfloat=1;
                    mem[A+mbase].shi=run[rtop].shi;
                }
                else{
                    mem[A+mbase].isfloat=0;
                    mem[A+mbase].zheng=run[rtop].zheng;
                }
            }
            }
            else{
                if(l==0){
                    if(run[rtop].isfloat==mem[A].isfloat){
                        if(run[rtop].isfloat){
                        mem[A].shi=run[rtop].shi;
                    }
                    else{
                        mem[A].zheng=run[rtop].zheng;
                    }

                    }
                    else if(mem[A].isfloat==1){

                            mem[A].shi=run[rtop].zheng;

                    }
                    else{
                        error(35);
                    }
                }
                else{
                    if(run[rtop].isfloat==mem[A+mbase].isfloat){
                        if(run[rtop].isfloat){
                        mem[A+mbase].shi=run[rtop].shi;
                    }
                    else{
                        mem[A+mbase].zheng=run[rtop].zheng;
                    }
                    }
                    else if(mem[A+mbase].isfloat==1){
                        mem[A+mbase].shi=run[rtop].zheng;
                    }
                    else{
                        error(35);
                    }
                }
            }
            pc++;
            rtop--;
            break;
        //QUT 0 0,存储栈退栈一次
        case QUT:
            mtop=mbase;
            mbase=mem[mbase+1].zheng;
            pc++;
            break;
         //rei 0 0 读取指令，从标准输入读取整数，于栈顶
        case rei:
            rtop++;
            run[rtop].isfloat=0;
            scanf("%d",&run[rtop].zheng);
            pc++;
            break;
        case ref:
            rtop++;
            run[rtop].isfloat=1;
            scanf("%lf",&run[rtop].shi);
            pc++;
            break;
        case rec:
            rtop++;
            run[rtop].isfloat=0;
            scanf("%c",&run[rtop].zheng);
            pc++;
            break;
        //wr 0 0标准输出指令，输出栈顶至标准输出
        case wr:
            if(p_code[pc].par2.zheng==2){
                if(run[rtop].isfloat){
                    printf("%lf",run[rtop].shi);
                }
                else{
                     printf("%lf",(double)run[rtop].zheng);
                }
            }
            else{
                if(p_code[pc].par2.zheng==1){
                    printf("%d",run[rtop].zheng);
                }
                else if(p_code[pc].par2.zheng==0){
                    putchar(run[rtop].zheng);
                }

            }
            rtop--;
            pc++;
            break;
        case ws:
            A=p_code[pc].par2.zheng;
            printf(hc[A]);
            pc++;
            break;
        }
    }
}
int main()
{
    int x=0;
    int i=0;
    char C0_address[100];
    puts("请输入源代码路径：");
    gets(C0_address);

    if((C0_code=fopen(C0_address,"r"))==NULL){
        puts("C0代码文件位置有误");
        exit(1);
    }
    program();
    //for(i=0;i<p_top;i++){
    //      printf("%d: %s,%d,%d,,,%d\n",i,P_code_name_name[p_code[i].id],p_code[i].par1.zheng,p_code[i].par2.zheng,p_code[i].isfloat);
    //}
    //for(i=1;i<=tabtop;i++){
    //    printf("%s,%d,%d\n",tab[i].name,tab[i].kind,tab[i].type);
    //}
    puts("\n结果在这!：");
    if(is_error==0){
        interupt();
    }

}
