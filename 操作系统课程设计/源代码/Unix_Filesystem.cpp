#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;


//字符串分割函数
vector<string> split(const string &s, const string &seperator) {
	vector<string> result;
	typedef string::size_type string_size;
	string_size i = 0;

	while (i != s.size()) {
		//找到字符串中首个不等于分隔符的字母；
		int flag = 0;
		while (i != s.size() && flag == 0) {
			flag = 1;
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[i] == seperator[x]) {
					++i;
					flag = 0;
					break;
				}
		}

		//找到又一个分隔符，将两个分隔符之间的字符串取出；
		flag = 0;
		string_size j = i;
		while (j != s.size() && flag == 0) {
			for (string_size x = 0; x < seperator.size(); ++x)
				if (s[j] == seperator[x]) {
					flag = 1;
					break;
				}
			if (flag == 0)
				++j;
		}
		if (i != j) {
			result.push_back(s.substr(i, j - i));
			i = j;
		}
	}
	return result;
}

/*整个虚拟文件卷共占87个物理盘块，每个物理盘块为512B/
/*SuperBlock占一个物理盘快，INode区占10个物理盘块，DiskNode区占76个物理盘块*/
/*本二级文件系统为单用户系统，只允许一个用户使用*/

#define numOfInode 80
#define numOfDisknode 76
#define read_only 0
#define write_only 1
#define read_write 2
#define dirfile 1
#define normalfile 0

fstream Disk;


bool isEmpty = true;

//文件系统控制块，大小固定为200+4+304+4=512B，即一个物理盘块
struct SuperBlock
{
	int freeINode[50];		//管理空闲INode,保存空闲INode编号
	int numOfFreeINode;		//剩余空闲INode数量
	
	int freeDiskNode[76];	//管理空闲DiskNode,保存空闲DiskNode编号
	int numOfFreeDiskNode;	//剩余空闲DiskNode数量
};


//文件控制块，每个文件对应一个INode,每一个INode块占64B
typedef struct INode 
{
	char filename[28];	//文件名
	int index;			//INode块编号
	int addr[5];		//文件索引表，采用二级索引，存放逻辑块号对应的物理块号
	int mode;			//文件打开模式，分为只读、只写、可读写，即0、1、2
	int file_type;		//文件类型，分为普通文件和目录文件，对应0、1
	int numOfSubPath;	//子目录/文件个数

}INode, INodes[numOfInode];


//物理盘块，一个盘块512B
typedef struct DiskNode
{
	char content[512];
		
}DiskNode, DiskNodes[numOfDisknode];


//用户打开文件表
typedef struct UOT
{
	int list[10];
	int numOfOpenedFile = 0;
	int offset[10] = { 0 };
}UOT;

SuperBlock spblc;
INodes inodes;
DiskNodes disknodes;
UOT uot;


//根据文件路径找到对应地INode节点的编号
int SearchPath(string filepath)
{
	vector<string> v = split(filepath, "/");
	int lastInodeIndex = 1;		//当前路径上一级目录的INode节点编号
	for (vector<string>::size_type i = 1; i != v.size(); ++i)
	{
		if (inodes[lastInodeIndex - 1].file_type == dirfile)
		{
			bool found = false;
			for (int j = 0; j < 5; j++)
			{
				
				for (int k = 0; k < 512 / 32; k++)		//取出每一项进行比对
				{
					char filename_index[32] = { 0 };
					memcpy(filename_index, disknodes[inodes[lastInodeIndex - 1].addr[j] - 1].content + k * 32, 32);
					//strncpy(filename_index, disknodes[inodes[lastInodeIndex - 1].addr[j] - 1].content + k * 32,32);
					int length = v[i].length();
					if (strncmp(filename_index, v[i].c_str(), length) == 0)
					{
						found = true;
						memcpy(&lastInodeIndex, filename_index + 28, 4);
						break;
					}
				}
				if (found)
					break;
			}
			if (!found)		//没有在父节点目录中找到子节点
				return -1;

		}
		else     //出现中间路径不是目录文件，出错
		{
			return -1;
		}
	}
	return lastInodeIndex;

}

//分配INode节点
int AllocINode()
{
	int numofFreeInodes = spblc.numOfFreeINode;
	if (numofFreeInodes > 0)
	{
		spblc.numOfFreeINode--;
		return spblc.freeINode[spblc.numOfFreeINode+1];		//加1是因为1号INode固定分配给根目录文件
	}
	else
	{
		return -1;
	}
}


//分配磁盘空间
int AllocMemory()
{
	int numOfFreeDisknodes = spblc.numOfFreeDiskNode;
	if (numOfFreeDisknodes > 0)
	{
		spblc.numOfFreeDiskNode--;
		return spblc.freeDiskNode[spblc.numOfFreeDiskNode];
	}
	else
	{
		return -1;
	}
}


//回收磁盘空间
void RetrieveMemory(int indexOfDisknode)
{
	if (indexOfDisknode != -1)
	{
		memset(disknodes[indexOfDisknode - 1].content, 0, 512);
		spblc.freeDiskNode[spblc.numOfFreeDiskNode] = indexOfDisknode;
		spblc.numOfFreeDiskNode++;
	}
}

//回收INode
void RetrieveINode(int indexofINode)
{
	memset(inodes[indexofINode - 1].filename, 0, 28);
	strncpy(inodes[indexofINode - 1].filename, "null", 4);
	inodes[indexofINode - 1].numOfSubPath = 0;
	inodes[indexofINode - 1].mode = -1;
	inodes[indexofINode - 1].file_type = normalfile;
	spblc.freeINode[spblc.numOfFreeINode] = indexofINode;
	spblc.numOfFreeINode++;
}

//格式化文件卷
void fformat()
{
	//初始化DiskiNodes
	for (int i = 0; i < numOfDisknode; ++i)
		memset(disknodes[i].content, 0, 512);

	//初始化INodes
	for (int i = 0; i < numOfInode; ++i)
	{
		strcpy(inodes[i].filename, "null");
		inodes[i].index = i + 1;
		inodes[i].mode = -1;
		inodes[i].file_type = normalfile;
		inodes[i].numOfSubPath = 0;
		for (int j = 0; j < 5; ++j)
			inodes[i].addr[j] = -1;
	}

	//初始化SuperBlock
	spblc.numOfFreeINode = 50;
	spblc.numOfFreeDiskNode = 76;
	for (int i = 0; i < 50; ++i)
		spblc.freeINode[i] = i + 1;
	for (int j = 0; j < 76; ++j)
		spblc.freeDiskNode[j] = j + 1;

	//初始化根目录文件
	strcpy(inodes[0].filename, "root");
	inodes[0].mode = read_write;
	inodes[0].file_type = dirfile;
	inodes[0].numOfSubPath = 0;
	for (int i = 0; i < 5; ++i)
		inodes[0].addr[i] = AllocMemory();
	spblc.numOfFreeINode--;
}

//初始化文件系统
void InitFileSystem()
{
	
	if (isEmpty == true)	//第一次挂载文件卷
	{
		//初始化DiskiNodes
		for (int i = 0; i < numOfDisknode; ++i)
			memset(disknodes[i].content, 0, 512);

		//初始化INodes
		for (int i = 0; i < numOfInode; ++i)
		{
			strcpy(inodes[i].filename, "null");
			inodes[i].index = i + 1;
			inodes[i].mode = -1;
			inodes[i].file_type = normalfile;
			inodes[i].numOfSubPath = 0;
			for (int j = 0; j < 5; ++j)
				inodes[i].addr[j] = -1;
		}

		//初始化SuperBlock
		spblc.numOfFreeINode = 50;
		spblc.numOfFreeDiskNode = 76;
		for (int i = 0; i < 50; ++i)
			spblc.freeINode[i] = i + 1;
		for (int j = 0; j < 76; ++j)
			spblc.freeDiskNode[j] = j + 1;

		//初始化根目录文件
		strcpy(inodes[0].filename, "root");
		inodes[0].mode = read_write;
		inodes[0].file_type = dirfile;
		inodes[0].numOfSubPath = 0;
		for (int i = 0; i < 5; ++i)
			inodes[0].addr[i] = AllocMemory();
		spblc.numOfFreeINode--;

		Disk.write((const char*)(&spblc), sizeof(spblc));
		for (int i = 0; i < numOfInode; ++i)
			Disk.write((const char*)(&inodes[i]), sizeof(INode));
		for (int i = 0; i < numOfDisknode; ++i)
			Disk.write((const char*)(&disknodes[i]), sizeof(DiskNode));
		//Disk.flush();
	}
	else		//文件卷已存在
	{
		//从文件卷读入
		Disk.read((char*)(&spblc), sizeof(spblc));
		for (int i = 0; i < numOfInode; ++i)
			Disk.read(( char*)(&inodes[i]), sizeof(INode));
		for (int i = 0; i < numOfDisknode; ++i)
			Disk.read(( char*)(&disknodes[i]), sizeof(DiskNode));

		
	}
}


//创建文件,返回文件INode节点编号
int createFile(string filename, int file_type, int mode)
{
	int s = SearchPath(filename);
	if (s != -1)
	{
		cout << "文件已存在......" << endl;
		return s;
	}
	if (spblc.numOfFreeINode > 0)
	{
		vector<string> v = split(filename, "/");
		string name = v[v.size() - 1];
		string parent_path = filename.substr(0, filename.length() - name.length() - 1);
		int parent_index = SearchPath(parent_path);
		if (parent_index == -1)		//路径错误情况1
		{
			cout << "文件路径违法......" << endl;
			return -1;
		}
		if (inodes[parent_index-1].file_type!=dirfile)		//路径错误情况2
		{
			cout << "文件路径违法......" << endl;
			return -1;
		}
		else			//父级目录存在
		{

			int k = inodes[parent_index - 1].numOfSubPath / (512 / 32);
			int m = inodes[parent_index - 1].numOfSubPath % (512 / 32);
			if (k >= 5)
			{
				cout << "无法创建文件,父目录以达到最大文件数" << endl;
				return -1;
			}

			int index = AllocINode();
			strcpy(inodes[index - 1].filename, name.c_str());
			inodes[index - 1].file_type = file_type;
			inodes[index - 1].mode = mode;
			inodes[index - 1].numOfSubPath = 0;

			if (inodes[parent_index - 1].addr[k] != -1 && k<5)
			{
				char t[28] = { 0 };
				strncpy(t, name.c_str(), name.length());
				memcpy(disknodes[inodes[parent_index - 1].addr[k] - 1].content + m * 32, t, 28);
				memcpy(disknodes[inodes[parent_index - 1].addr[k] - 1].content + m * 32 + 28, &index, 4);
				inodes[parent_index - 1].numOfSubPath++;
				
			}
			else if(inodes[parent_index - 1].addr[k]==-1 && k<5)
			{
				inodes[parent_index - 1].addr[k] = AllocMemory();
				char t[28] = { 0 };
				strncpy(t, name.c_str(), name.length());
				memcpy(disknodes[inodes[parent_index - 1].addr[k] - 1].content + m * 32, t, 28);
				memcpy(disknodes[inodes[parent_index - 1].addr[k] - 1].content + m * 32 + 28, &index, 4);
				inodes[parent_index - 1].numOfSubPath++;

			}
			
			return index;
		}
	}
	else
	{
		cout << "无法创建文件......" << endl;
		return -1;
	}
}

//删除文件
bool deleteFile(string filename)
{
	int index = SearchPath(filename);
	if(index!=-1)
	{
		for (int i = 0; i < 5; i++)			//回收该文件占用的Disknode
			RetrieveMemory(inodes[index - 1].addr[i]);

		/*回收该文件占用的INode*/
		RetrieveINode(index);

		/*删除该文件在父目录中的项*/
		vector<string> v = split(filename, "/");
		string name = v[v.size() - 1];
		string parent_path = filename.substr(0, filename.length() - name.length() - 1);
		int parent_index = SearchPath(parent_path);
		for (int i = 0; i < 5; i++)
		{
			bool found = false;
			for (int j = 0; j < 512 / 32; j++)
			{
				char filename_index[32] = { 0 };
				memcpy(filename_index, disknodes[inodes[parent_index - 1].addr[i] - 1].content + j * 32, 32);
				if (strncmp(filename_index, name.c_str(), name.length())==0)
				{
					found = true;
					int k = inodes[parent_index - 1].numOfSubPath / (512 / 32);
					int m = inodes[parent_index - 1].numOfSubPath % (512 / 32);
					if (m > 0)
					{
						memcpy(disknodes[inodes[parent_index - 1].addr[i] - 1].content + j * 32, disknodes[inodes[parent_index - 1].addr[k] - 1].content + (m - 1) * 32, 32);
						memset(disknodes[inodes[parent_index - 1].addr[k] - 1].content + (m - 1) * 32, 0, 32);
					}
					else
					{
						memcpy(disknodes[inodes[parent_index - 1].addr[i] - 1].content + j * 32, disknodes[inodes[parent_index - 1].addr[k - 1] - 1].content + 15 * 32, 32);
						memset(disknodes[inodes[parent_index - 1].addr[k - 1] - 1].content + 15 * 32, 0, 32);
					}
					break;
				}
			}
			if (found)
				break;
		}
		inodes[parent_index - 1].numOfSubPath--;
		return true;
	}
	else
	{
		cout << "此文件或目录不存在......" << endl;
		return false;
	}
}
 
//打开文件,返回文件INode编号
int openFile(string filename,int mode)
{
	int index = SearchPath(filename);
	if (index != -1 && inodes[index-1].mode==mode)
	{
		cout << "打开文件成功......" << endl;

		for (int i = 0; i < uot.numOfOpenedFile; i++)
			if (uot.list[i] == index)
				return uot.list[i];

		/*更新用户打开文件表*/
		uot.list[uot.numOfOpenedFile] = index;
		uot.offset[uot.numOfOpenedFile] = 0;
		uot.numOfOpenedFile++;

		return index;
	}
	else if (index != -1 && inodes[index - 1].mode != mode)
	{
		cout << "违法的打开权限......" << endl;
		return -1;
	}
	else
	{
		cout << "文件不存在，无法打开......" << endl;
		return -1;
	}
}


//读取文件,返回实际读取的字节数,重载1
int readFile(string filename,char* buffer,int length)
{
	int index = SearchPath(filename);
	if (index != -1)
	{
		bool isopened = false;
		int a = -1;
		int offset = -1;
		for (int i = 0; i < uot.numOfOpenedFile; i++)
			if (uot.list[i] == index)
			{
				a = i;
				offset = uot.offset[i];
				isopened = true;
				break;
			}
		if (isopened)
		{
			if (inodes[index - 1].mode == read_only || inodes[index - 1].mode == read_write)	//只读或可读写,符合权限要求
			{
				int k = offset / 512;
				if (inodes[index - 1].addr[k] != -1)
				{
					int m = offset % 512;
					int left = length;
					int buffer_offset = 0;
					while (left > 0 && inodes[index-1].addr[k]!=-1 && k<5)		//还有剩余字节没读取完
					{
						if (left > (512 - m))
						{
							memcpy(buffer + buffer_offset, disknodes[inodes[index - 1].addr[k] - 1].content + m, 512 - m);
							left -= (512 - m);
							buffer_offset += (512 - m);
							m = 0;
							k++;
							
						}
						else
						{
							memcpy(buffer + buffer_offset, disknodes[inodes[index - 1].addr[k] - 1].content + m, left);
							m += left;
							left -= left;
						}
					}
					if (left == 0)		//全部读取完毕
					{
						cout << "读取成功......" << endl;
						uot.offset[a] += length;
						return length;
					}
					else                //读取部分内容
					{
						cout << "读取成功......" << endl;
						uot.offset[a] += (length - left);
						return (length - left);
					}
				}
				else    //超出文件内容范围
				{
					cout << "无法读取......" << endl;
					return 0;
				}
			}
			else		//只写，不符合权限要求
			{
				cout << "违法的权限......" << endl;
				return -1;
			}
		}
		else
		{
			cout << "文件尚未打开......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "文件不存在......" << endl;
		return -1;
	}
}

//读取文件，返回实际读取的字节数，重载2
int readFile(int index, char* buffer, int length)
{
	bool isopened = false;
	int a = -1;
	int offset = -1;
	for (int i = 0; i < uot.numOfOpenedFile; i++)
		if (uot.list[i] == index)
		{
			a = i;
			offset = uot.offset[i];
			isopened = true;
			break;
		}
	if (isopened)
	{
		if (inodes[index - 1].mode == read_only || inodes[index - 1].mode == read_write)	//只读或可读写,符合权限要求
		{
			int k = offset / 512;
			if (inodes[index - 1].addr[k] != -1)
			{
				int m = offset % 512;
				int left = length;		//剩余待读取字节数
				int buffer_offset = 0;	//数组偏移
				while (left > 0 && inodes[index - 1].addr[k] != -1 && k<5)		//还有剩余字节没读取完
				{
					if (left >(512 - m))
					{
						memcpy(buffer + buffer_offset, disknodes[inodes[index - 1].addr[k] - 1].content + m, 512 - m);
						left -= (512 - m);
						buffer_offset += (512 - m);
						m = 0;
						k++;				
					}
					else
					{
						memcpy(buffer + buffer_offset, disknodes[inodes[index - 1].addr[k] - 1].content + m, left);
						m += left;
						left -= left;
					}
				}
				if (left == 0)		//全部读取完毕
				{
					cout << "读取成功......" << endl;
					uot.offset[a] += length;
					return length;
				}
				else                //读取部分内容
				{
					cout << "读取部分内容成功......" << endl;
					uot.offset[a] += (length - left);
					return (length - left);
				}
			}
			else    //超出文件内容范围
			{
				cout << "无法读取......" << endl;
				return 0;
			}
		}
		else		//只写，不符合权限要求
		{
			cout << "违法的权限......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "文件尚未打开......" << endl;
		return -1;
	}
}


//写文件，返回实际写入的字节数，重载1
int writeFile(string filename, const char* buffer, int length)
{
	int index = SearchPath(filename);
	if (index != -1)
	{
		bool isopened = false;
		int a = -1;
		int offset = -1;
		for (int i = 0; i < uot.numOfOpenedFile; i++)
			if (uot.list[i] == index)
			{
				a = i;
				offset = uot.offset[i];
				isopened = true;
				break;
			}
		if (isopened)
		{
			if (inodes[index - 1].mode == write_only || inodes[index - 1].mode == read_write)
			{
				int k = offset / 512;
				int m = offset % 512;
				if (k >= 5)
				{
					cout << "无法写入......" << endl;
					return 0;
				}
				if (inodes[index - 1].addr[k] == -1)
				{
					inodes[index - 1].addr[k] = AllocMemory();
					if (inodes[index - 1].addr[k] == -1)
					{
						cout << "空间不足,无法写入数据......" << endl;
						return 0;
					}
				}
				if (inodes[index - 1].addr[k] != -1)
				{
					int left = length;
					int buffer_offset = 0;
					while (left > 0 && k<5)
					{
						if (left > (512 - m) && inodes[index - 1].addr[k] != -1)		//
						{
							memcpy(disknodes[inodes[index - 1].addr[k] - 1].content + m, buffer + buffer_offset, (512 - m));
							k++;
							buffer_offset += (512 - m);
							left -= (512 - m);
							m = 0;
							
						}
						else if(left<=(512-m) && inodes[index-1].addr[k]!=-1)
						{
							memcpy(disknodes[inodes[index - 1].addr[k] - 1].content + m, buffer + buffer_offset, left);
							left -= left;
						}
						else if (inodes[index - 1].addr[k] == -1)
						{
							inodes[index - 1].addr[k] = AllocMemory();
							if (inodes[index - 1].addr[k] == -1)
							{
								cout << "空间不足，写入部分数据......" << endl;
								return (length - left);
							}
							
						}
					}
					if (left == 0)		//数据全部写入文件
					{
						cout << "写入成功......" << endl;
						uot.offset[a] += length;
						return length;
					}
					else if (k >= 5)	//超出文件尾部，只写入了部分
					{
						cout << "写入成功,写入部分数据......" << endl;
						uot.offset[a] += (length - left);
						return (length - left);
					}
				}
			}
			else
			{
				cout << "违法的权限......" << endl;
				return -1;
			}
		}
		else
		{
			cout << "文件未打开......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "文件不存在......" << endl;
		return -1;
	}
}


//写文件，返回实际写入的字节数，重载2
int writeFile(int index, const char* buffer, int length)
{
	bool isopened = false;
	int a = -1;
	int offset = -1;
	for (int i = 0; i < uot.numOfOpenedFile; i++)
		if (uot.list[i] == index)
		{
			a = i;
			offset = uot.offset[i];
			isopened = true;
			break;
		}
	if (isopened)
	{
		if (inodes[index - 1].mode == write_only || inodes[index - 1].mode == read_write)
		{
			int k = offset / 512;
			int m = offset % 512;
			if (k >= 5)
			{
				cout << "无法写入......" << endl;
				return 0;
			}
			if (inodes[index - 1].addr[k] == -1)
			{
				inodes[index - 1].addr[k] = AllocMemory();
				if (inodes[index - 1].addr[k] == -1)
				{
					cout << "空间不足,无法写入数据......" << endl;
					return 0;
				}
			}
			if (inodes[index - 1].addr[k] != -1)
			{
				int left = length;
				int buffer_offset = 0;
				while (left > 0 && k<5)
				{
					if (left >(512 - m) && inodes[index - 1].addr[k] != -1)		//
					{
						memcpy(disknodes[inodes[index - 1].addr[k] - 1].content + m, buffer + buffer_offset, (512 - m));
						k++;
						buffer_offset += (512 - m);
						left -= (512 - m);
						m = 0;				
					}
					else if (left <= (512 - m) && inodes[index - 1].addr[k] != -1)
					{
						memcpy(disknodes[inodes[index - 1].addr[k] - 1].content + m, buffer + buffer_offset, left);
						left -= left;
					}
					else if (inodes[index - 1].addr[k] == -1)
					{
						inodes[index - 1].addr[k] = AllocMemory();
						if (inodes[index - 1].addr[k] == -1)
						{
							cout << "空间不足，写入部分数据......" << endl;
							return (length - left);
						}

					}
				}
				if (left == 0)		//数据全部写入文件
				{
					cout << "写入成功......" << endl;
					uot.offset[a] += length;
					return length;
				}
				else if (k >= 5)	//超出文件尾部，只写入了部分
				{
					cout << "写入成功,写入部分数据......" << endl;
					uot.offset[a] += (length - left);
					return (length - left);
				}
			}
		}
		else
		{
			cout << "违法的权限......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "文件未打开......" << endl;
		return -1;
	}
}

//移动文件指针，重载1
void seekPos(string filename, int offset)
{
	int index = SearchPath(filename);
	if (index != -1)
	{
		bool isopened = false;
		for (int i = 0; i < uot.numOfOpenedFile; i++)
			if (uot.list[i] == index)
			{
				isopened = true;
				uot.offset[i] = offset;
				break;
			}
		if(!isopened)
			cout << "文件未打开,无法移动文件指针......" << endl;
	}
	else
		cout << "文件不存在......" << endl;
}


//移动文件指针，重载2
int seekPos(int index, int offset)
{
	bool isopened = false;
	for (int i = 0; i < uot.numOfOpenedFile; i++)
		if (uot.list[i] == index)
		{
			isopened = true;
			uot.offset[i] = offset;
			return 1;
		}
	if (!isopened)
	{
		cout << "文件未打开,无法移动文件指针......" << endl;
		return 0;
	}
}

//获取文件指针位置，重载1
int getPos(string filename)
{
	int index = SearchPath(filename);
	if (index != -1)
	{
		for (int i = 0; i < uot.numOfOpenedFile; i++)
			if (uot.list[i] == index)
				return uot.offset[i];
		return -1;
	}
	else
		return -2;
}

//获取文件指针，重载2
int getPos(int index)
{
	for (int i = 0; i < uot.numOfOpenedFile; i++)
		if (uot.list[i] == index)
			return uot.offset[i];
	return -1;
}


//关闭文件
void closeFile(int index)
{
	bool isopened = false;
	for (int i = 0; i < uot.numOfOpenedFile; i++)
	{
		if (uot.list[i] == index)
		{
			for (int j = i; j < uot.numOfOpenedFile - 1; j++)
			{
				uot.list[j] = uot.list[j + 1];
				uot.offset[j] = uot.offset[j + 1];
			}
			uot.numOfOpenedFile--;
		}
	}
}

//列目录
void ls()
{
	int i, j, k;
	cout << "root/" << endl;
	for (i = 0; i < inodes[0].numOfSubPath; i++)
	{
		j = i / (512 / 32);
		k = i % (512 / 32);
		char filename_index[32] = { 0 };
		memcpy(filename_index, disknodes[inodes[0].addr[j] - 1].content + k * 32, 32);
		int index;
		memcpy(&index, filename_index + 28, 4);
		char filename[28] = { 0 };
		memcpy(filename, filename_index, 28);
		cout << "-----";
		cout << inodes[index - 1].filename;
		cout << "  -" << (inodes[index - 1].file_type==0?"普通文件":"目录文件");
		cout << endl;
	}
}


//用户输入命令，程序返回结果
void InputCommand()
{
	bool flag = false;
	cout << "******************************************" << endl;
	cout << "文件系统已启动,按Q退出......" << endl;
	cout << "1.创建文件" << endl;
	cout << "2.打开文件" << endl;
	cout << "3.读取文件" << endl;
	cout << "4.写入文件" << endl;
	cout << "5.删除文件" << endl;
	cout << "6.列出根目录下所有文件和目录" << endl;
	cout << "7.格式化文件卷" << endl;
	cout << "8.清屏(保留指令提示)" << endl;
	cout << "******************************************" << endl;
	while (1)
	{
		if (flag)
		{
			cout << "******************************************" << endl;
			cout << "文件系统已启动,按Q退出......" << endl;
			cout << "1.创建文件" << endl;
			cout << "2.打开文件" << endl;
			cout << "3.读取文件" << endl;
			cout << "4.写入文件" << endl;
			cout << "5.删除文件" << endl;
			cout << "6.列出根目录下所有文件和目录" << endl;
			cout << "7.格式化文件卷" << endl;
			cout << "8.清屏(保留指令提示)" << endl;
			cout << "******************************************" << endl;
		}
		cout << "输入你想执行的指令：" << endl;
		char choice;
		char is800;
		string filename;
		string write_content;
		int filetype;
		int mode;
		int index;
		int offset = 0;
		int fd = -1;
		char* buffer = NULL;
		int length = 0;
		int read_len = 0;
		int write_len = 0;
		char test[800] = { 0 };
		for (int i = 0; i < 800; i++)
			test[i] = '0' + i % 10;
		cin >> choice;
		switch (choice)
		{
		case '1':		//创建文件
			flag = false;
			cout << "输入文件名(格式为root/xxx/xxx,root为根目录)：";
			cin >> filename;
			cout << "输入文件类型(目录文件1或者普通文件0):";
			cin >> filetype;
			cout << "输入文件读写方式(只读0，只写1，可读写2):";
			cin >> mode;
		    index = createFile(filename, filetype, mode);
			if (index != -1)
			{
				cout << filename << "创建成功！" << endl;
				cout << "文件句柄fd为：" << index << endl;
			}
			else
				cout << "创建文件失败！" << endl;
			break;
		case '2':		//打开文件
			flag = false;

			cout << "输入文件名(格式为root/xxx/xxx,root为根目录):";
			cin >> filename;
			cout << "输入打开方式(只读0，只写1，可读写2):";
			cin >> mode;
			index = openFile(filename, mode);
			if (index != -1)
			{
				cout << filename << "已打开，可进行";
				if (mode == read_only)
					cout << "读操作" << endl;
				else if (mode == write_only)
					cout << "写操作" << endl;
				else
					cout << "读写操作" << endl;
				cout << "文件句柄fd为：" << index << endl;
			}
			else
				cout << "违法的权限或者文件不存在" << endl;
			break;
		case '3':		//读文件
			flag = false;
			cout << "输入文件句柄:";
			cin >> fd;
			cout << "文件指针偏移:";
			cin >> offset;
			cout << "输入读取的字节数:";
			cin >> length;
			if (length <= 0)
				cout << "无效的读操作" << endl;
			else
			{
				buffer = new char[length+1];	//根据用户读取的字节数申请空间
				memset(buffer, 0, length + 1);
				seekPos(fd, offset);
				read_len = readFile(fd, buffer, length);
				if (read_len > 0)
				{
					cout << "读取了" << read_len << "字节......" << endl;
					cout << "文件指针当前位置:" << getPos(fd) << endl;
					char ch;
					cout << "是否显示读取内容(Y/N):";
					cin >> ch;
					if (ch == 'Y' || ch == 'y')
					{
						cout << buffer << endl;
					}
					else if(ch=='N'||ch=='n')
					{
						break;
					}
					delete buffer;
				}
			}
			break;
		case '4':		//写文件
			flag = false;
			cout << "输入文件句柄:";
			cin >> fd;
			cout << "文件指针偏移:";
			cin >> offset;
			cout << "是否写入长度为800字节的测试数据(Y/N):";
			cin >> is800;
			if (is800 == 'Y' || is800 == 'y')
			{
				seekPos(fd, offset);
				write_len = writeFile(fd, test, 800);
				if (write_len == 800)
				{
					cout << "写入测试数据成功" << endl;
					cout << "文件指针当前位置:" << getPos(fd) << endl;
				}
			}
			else
			{
				cout << "输入欲写入的内容:";
				cin >> write_content;
				length = write_content.length();
				if (length <= 0)
					cout << "无效的操作" << endl;
				else
				{
					seekPos(fd, offset);
					write_len = writeFile(fd, write_content.c_str(), length);
					if (write_len > 0)
					{
						cout << "写入了" << write_len << "字节......" << endl;
						cout << "文件指针当前位置:" << getPos(fd) << endl;
					}
				}
			}	
			break;
		case '5':		//删除文件
			flag = false;
			cout << "输入文件名(格式为root/xxx/xxx,root为根目录):";
			cin >> filename;
			if (deleteFile(filename) == true)
				cout << "删除" << filename << "成功" << endl;
			break;
		case '6':
			ls();
			flag = false;
			break;
		case '7':
			fformat();
			cout << "格式化文件卷成功" << endl;
			flag = false;
			break;
		case '8':
			system("cls");
			flag = true;
			break;
		case 'Q':
		case 'q':
			return;
		default:
			cout << "不合法的输入......" << endl;
			break;
		}
	}
}

//每一次用户操作完成之后，保存
void Save()
{
	Disk.seekp(0);
	Disk.write((const char*)(&spblc), sizeof(spblc));
	for (int i = 0; i < numOfInode; ++i)
		Disk.write((const char*)(&inodes[i]), sizeof(INode));
	for (int i = 0; i < numOfDisknode; ++i)
		Disk.write((const char*)(&disknodes[i]), sizeof(DiskNode));
}


//主函数
int main()
{
	ifstream test("disk", ios::in|ios::binary);
	if (!test.is_open())
		isEmpty = true;	
	else
		isEmpty = false;
	test.close();

	if (!isEmpty)
	{
		Disk.open("disk", ios::in | ios::out |ios::binary);
		if (!Disk.is_open())
		{
			cout << "文件卷加载失败!" << endl;
			return -1;
		}
		InitFileSystem();
		InputCommand();
	}
	else
	{
		Disk.open("disk", ios::out|ios::binary);
		InitFileSystem();
		Disk.close();
		Disk.open("disk.txt", ios::in | ios::out | ios::binary);
		if (!Disk.is_open())
		{
			cout << "文件卷加载失败!" << endl;
			return -1;
		}
		InputCommand();
	}
	Save();
	Disk.close();
	return 0;
}