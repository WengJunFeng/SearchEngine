#include "Statistics/src/Application.hpp"
#include "../inc/InvertIndex.hpp"
#include "../inc/IndexOffset.hpp"
#include "../inc/PageOffset.hpp"
#include "../inc/Configure.hpp"
#include "../inc/IDF.hpp"

#include "../inc/InetAddress.h"
#include "../inc/Threadpool.h"
#include "../inc/SocketIO.h"
#include "../inc/Socket.h"
#include "../inc/Task.h"
#include <unistd.h>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#define LOG4CPP
#include "../inc/Mylog.h"

#if 1
#include <iostream>
#endif

using namespace CppJieba;
int main(int argc,char* argv[])
{
	
	if(argc!=2)
	{
		perror("args error!");
		exit(1);
	}
	 CppJieba::Application *app = new CppJieba::Application
						   ("Statistics/dict/jieba.dict.utf8",
                            "Statistics/dict/hmm_model.utf8",
                            "Statistics/dict/user.dict.utf8",
                            "Statistics/dict/idf.utf8",
                            "Statistics/dict/stop_words.utf8");
	/*****************************************/
	//从命令行参数读取配置文件
	/*****************************************/
	Configure conf(argv[1]);	//加载配置文件
	//Configure conf("../conf/configure.txt");	//加载配置文件
	InvertIndex *index = new InvertIndex(conf.getConf("indexOffsetPath"),conf.getConf("invertIndexPath"));
	IDF *idf = new IDF(conf.getConf("idfPath"));	//加载逆文档频率文件
	string pageoffsetPath = conf.getConf("pagesOffsetPath");
	string pagelibPath = conf.getConf("pagesLibPath");

	/*****************************************/
	//初始化线程池
	/*****************************************/
	int bufSiz = atoi(conf.getConf("bufSiz").c_str());
	int threadNum = atoi(conf.getConf("threadNum").c_str());
	//Threadpool threadpool(bufSiz,threadNum,pageoffsetPath,pagelibPath,*app);
	Threadpool *threadpool = new Threadpool(bufSiz,threadNum,pageoffsetPath,pagelibPath,*app);
	(*threadpool).start();

	/*****************************************/
	//初始化UDP服务器
	/*****************************************/
	const string IP =conf.getConf("ip");
	uint16_t PORT = atoi(conf.getConf("port").c_str());
	InetAddress serverAddr(IP,PORT);
	InetAddress clientAddr;
	Socket socket;
	int servfd = socket.fd();
	socket.ready(serverAddr);
	char buf[32];

	/*****************************************/
	//循环接收请求，并封装成任务，并加入线程池
	/*****************************************/
	Task *ptask ;
	string connInfo;
	LOG_INFO("Server is ready to wait for connecting……");
	while(1)
	{
		SocketIO sockIO(servfd);
		memset(buf,'\0',32);
		sockIO.readn(buf,32,clientAddr);
		int len = strlen(buf);
		buf[len-1]='\0';
		std::string str(buf);
		/******************************
		 *log
		 */
		connInfo = " Connecting from "+clientAddr.ip()+" Query : "+str;
		LOG_INFO(connInfo.c_str());
		ptask = new Task(servfd,str,*app,clientAddr,*index,*idf);//封装成任务
		threadpool->addTask(ptask);		//将任务添加到线程池中
	}
	return 0;
}
