#define _CRT_SECURE_NO_WARNINGS
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
using namespace std;


//�ַ����ָ��
vector<string> split(const string &s, const string &seperator) {
	vector<string> result;
	typedef string::size_type string_size;
	string_size i = 0;

	while (i != s.size()) {
		//�ҵ��ַ������׸������ڷָ�������ĸ��
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

		//�ҵ���һ���ָ������������ָ���֮����ַ���ȡ����
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

/*���������ļ���ռ87�������̿飬ÿ�������̿�Ϊ512B/
/*SuperBlockռһ�������̿죬INode��ռ10�������̿飬DiskNode��ռ76�������̿�*/
/*�������ļ�ϵͳΪ���û�ϵͳ��ֻ����һ���û�ʹ��*/

#define numOfInode 80
#define numOfDisknode 76
#define read_only 0
#define write_only 1
#define read_write 2
#define dirfile 1
#define normalfile 0

fstream Disk;


bool isEmpty = true;

//�ļ�ϵͳ���ƿ飬��С�̶�Ϊ200+4+304+4=512B����һ�������̿�
struct SuperBlock
{
	int freeINode[50];		//�������INode,�������INode���
	int numOfFreeINode;		//ʣ�����INode����
	
	int freeDiskNode[76];	//�������DiskNode,�������DiskNode���
	int numOfFreeDiskNode;	//ʣ�����DiskNode����
};


//�ļ����ƿ飬ÿ���ļ���Ӧһ��INode,ÿһ��INode��ռ64B
typedef struct INode 
{
	char filename[28];	//�ļ���
	int index;			//INode����
	int addr[5];		//�ļ����������ö�������������߼���Ŷ�Ӧ��������
	int mode;			//�ļ���ģʽ����Ϊֻ����ֻд���ɶ�д����0��1��2
	int file_type;		//�ļ����ͣ���Ϊ��ͨ�ļ���Ŀ¼�ļ�����Ӧ0��1
	int numOfSubPath;	//��Ŀ¼/�ļ�����

}INode, INodes[numOfInode];


//�����̿飬һ���̿�512B
typedef struct DiskNode
{
	char content[512];
		
}DiskNode, DiskNodes[numOfDisknode];


//�û����ļ���
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


//�����ļ�·���ҵ���Ӧ��INode�ڵ�ı��
int SearchPath(string filepath)
{
	vector<string> v = split(filepath, "/");
	int lastInodeIndex = 1;		//��ǰ·����һ��Ŀ¼��INode�ڵ���
	for (vector<string>::size_type i = 1; i != v.size(); ++i)
	{
		if (inodes[lastInodeIndex - 1].file_type == dirfile)
		{
			bool found = false;
			for (int j = 0; j < 5; j++)
			{
				
				for (int k = 0; k < 512 / 32; k++)		//ȡ��ÿһ����бȶ�
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
			if (!found)		//û���ڸ��ڵ�Ŀ¼���ҵ��ӽڵ�
				return -1;

		}
		else     //�����м�·������Ŀ¼�ļ�������
		{
			return -1;
		}
	}
	return lastInodeIndex;

}

//����INode�ڵ�
int AllocINode()
{
	int numofFreeInodes = spblc.numOfFreeINode;
	if (numofFreeInodes > 0)
	{
		spblc.numOfFreeINode--;
		return spblc.freeINode[spblc.numOfFreeINode+1];		//��1����Ϊ1��INode�̶��������Ŀ¼�ļ�
	}
	else
	{
		return -1;
	}
}


//������̿ռ�
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


//���մ��̿ռ�
void RetrieveMemory(int indexOfDisknode)
{
	if (indexOfDisknode != -1)
	{
		memset(disknodes[indexOfDisknode - 1].content, 0, 512);
		spblc.freeDiskNode[spblc.numOfFreeDiskNode] = indexOfDisknode;
		spblc.numOfFreeDiskNode++;
	}
}

//����INode
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

//��ʽ���ļ���
void fformat()
{
	//��ʼ��DiskiNodes
	for (int i = 0; i < numOfDisknode; ++i)
		memset(disknodes[i].content, 0, 512);

	//��ʼ��INodes
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

	//��ʼ��SuperBlock
	spblc.numOfFreeINode = 50;
	spblc.numOfFreeDiskNode = 76;
	for (int i = 0; i < 50; ++i)
		spblc.freeINode[i] = i + 1;
	for (int j = 0; j < 76; ++j)
		spblc.freeDiskNode[j] = j + 1;

	//��ʼ����Ŀ¼�ļ�
	strcpy(inodes[0].filename, "root");
	inodes[0].mode = read_write;
	inodes[0].file_type = dirfile;
	inodes[0].numOfSubPath = 0;
	for (int i = 0; i < 5; ++i)
		inodes[0].addr[i] = AllocMemory();
	spblc.numOfFreeINode--;
}

//��ʼ���ļ�ϵͳ
void InitFileSystem()
{
	
	if (isEmpty == true)	//��һ�ι����ļ���
	{
		//��ʼ��DiskiNodes
		for (int i = 0; i < numOfDisknode; ++i)
			memset(disknodes[i].content, 0, 512);

		//��ʼ��INodes
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

		//��ʼ��SuperBlock
		spblc.numOfFreeINode = 50;
		spblc.numOfFreeDiskNode = 76;
		for (int i = 0; i < 50; ++i)
			spblc.freeINode[i] = i + 1;
		for (int j = 0; j < 76; ++j)
			spblc.freeDiskNode[j] = j + 1;

		//��ʼ����Ŀ¼�ļ�
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
	else		//�ļ����Ѵ���
	{
		//���ļ������
		Disk.read((char*)(&spblc), sizeof(spblc));
		for (int i = 0; i < numOfInode; ++i)
			Disk.read(( char*)(&inodes[i]), sizeof(INode));
		for (int i = 0; i < numOfDisknode; ++i)
			Disk.read(( char*)(&disknodes[i]), sizeof(DiskNode));

		
	}
}


//�����ļ�,�����ļ�INode�ڵ���
int createFile(string filename, int file_type, int mode)
{
	int s = SearchPath(filename);
	if (s != -1)
	{
		cout << "�ļ��Ѵ���......" << endl;
		return s;
	}
	if (spblc.numOfFreeINode > 0)
	{
		vector<string> v = split(filename, "/");
		string name = v[v.size() - 1];
		string parent_path = filename.substr(0, filename.length() - name.length() - 1);
		int parent_index = SearchPath(parent_path);
		if (parent_index == -1)		//·���������1
		{
			cout << "�ļ�·��Υ��......" << endl;
			return -1;
		}
		if (inodes[parent_index-1].file_type!=dirfile)		//·���������2
		{
			cout << "�ļ�·��Υ��......" << endl;
			return -1;
		}
		else			//����Ŀ¼����
		{

			int k = inodes[parent_index - 1].numOfSubPath / (512 / 32);
			int m = inodes[parent_index - 1].numOfSubPath % (512 / 32);
			if (k >= 5)
			{
				cout << "�޷������ļ�,��Ŀ¼�Դﵽ����ļ���" << endl;
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
		cout << "�޷������ļ�......" << endl;
		return -1;
	}
}

//ɾ���ļ�
bool deleteFile(string filename)
{
	int index = SearchPath(filename);
	if(index!=-1)
	{
		for (int i = 0; i < 5; i++)			//���ո��ļ�ռ�õ�Disknode
			RetrieveMemory(inodes[index - 1].addr[i]);

		/*���ո��ļ�ռ�õ�INode*/
		RetrieveINode(index);

		/*ɾ�����ļ��ڸ�Ŀ¼�е���*/
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
		cout << "���ļ���Ŀ¼������......" << endl;
		return false;
	}
}
 
//���ļ�,�����ļ�INode���
int openFile(string filename,int mode)
{
	int index = SearchPath(filename);
	if (index != -1 && inodes[index-1].mode==mode)
	{
		cout << "���ļ��ɹ�......" << endl;

		for (int i = 0; i < uot.numOfOpenedFile; i++)
			if (uot.list[i] == index)
				return uot.list[i];

		/*�����û����ļ���*/
		uot.list[uot.numOfOpenedFile] = index;
		uot.offset[uot.numOfOpenedFile] = 0;
		uot.numOfOpenedFile++;

		return index;
	}
	else if (index != -1 && inodes[index - 1].mode != mode)
	{
		cout << "Υ���Ĵ�Ȩ��......" << endl;
		return -1;
	}
	else
	{
		cout << "�ļ������ڣ��޷���......" << endl;
		return -1;
	}
}


//��ȡ�ļ�,����ʵ�ʶ�ȡ���ֽ���,����1
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
			if (inodes[index - 1].mode == read_only || inodes[index - 1].mode == read_write)	//ֻ����ɶ�д,����Ȩ��Ҫ��
			{
				int k = offset / 512;
				if (inodes[index - 1].addr[k] != -1)
				{
					int m = offset % 512;
					int left = length;
					int buffer_offset = 0;
					while (left > 0 && inodes[index-1].addr[k]!=-1 && k<5)		//����ʣ���ֽ�û��ȡ��
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
					if (left == 0)		//ȫ����ȡ���
					{
						cout << "��ȡ�ɹ�......" << endl;
						uot.offset[a] += length;
						return length;
					}
					else                //��ȡ��������
					{
						cout << "��ȡ�ɹ�......" << endl;
						uot.offset[a] += (length - left);
						return (length - left);
					}
				}
				else    //�����ļ����ݷ�Χ
				{
					cout << "�޷���ȡ......" << endl;
					return 0;
				}
			}
			else		//ֻд��������Ȩ��Ҫ��
			{
				cout << "Υ����Ȩ��......" << endl;
				return -1;
			}
		}
		else
		{
			cout << "�ļ���δ��......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "�ļ�������......" << endl;
		return -1;
	}
}

//��ȡ�ļ�������ʵ�ʶ�ȡ���ֽ���������2
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
		if (inodes[index - 1].mode == read_only || inodes[index - 1].mode == read_write)	//ֻ����ɶ�д,����Ȩ��Ҫ��
		{
			int k = offset / 512;
			if (inodes[index - 1].addr[k] != -1)
			{
				int m = offset % 512;
				int left = length;		//ʣ�����ȡ�ֽ���
				int buffer_offset = 0;	//����ƫ��
				while (left > 0 && inodes[index - 1].addr[k] != -1 && k<5)		//����ʣ���ֽ�û��ȡ��
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
				if (left == 0)		//ȫ����ȡ���
				{
					cout << "��ȡ�ɹ�......" << endl;
					uot.offset[a] += length;
					return length;
				}
				else                //��ȡ��������
				{
					cout << "��ȡ�������ݳɹ�......" << endl;
					uot.offset[a] += (length - left);
					return (length - left);
				}
			}
			else    //�����ļ����ݷ�Χ
			{
				cout << "�޷���ȡ......" << endl;
				return 0;
			}
		}
		else		//ֻд��������Ȩ��Ҫ��
		{
			cout << "Υ����Ȩ��......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "�ļ���δ��......" << endl;
		return -1;
	}
}


//д�ļ�������ʵ��д����ֽ���������1
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
					cout << "�޷�д��......" << endl;
					return 0;
				}
				if (inodes[index - 1].addr[k] == -1)
				{
					inodes[index - 1].addr[k] = AllocMemory();
					if (inodes[index - 1].addr[k] == -1)
					{
						cout << "�ռ䲻��,�޷�д������......" << endl;
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
								cout << "�ռ䲻�㣬д�벿������......" << endl;
								return (length - left);
							}
							
						}
					}
					if (left == 0)		//����ȫ��д���ļ�
					{
						cout << "д��ɹ�......" << endl;
						uot.offset[a] += length;
						return length;
					}
					else if (k >= 5)	//�����ļ�β����ֻд���˲���
					{
						cout << "д��ɹ�,д�벿������......" << endl;
						uot.offset[a] += (length - left);
						return (length - left);
					}
				}
			}
			else
			{
				cout << "Υ����Ȩ��......" << endl;
				return -1;
			}
		}
		else
		{
			cout << "�ļ�δ��......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "�ļ�������......" << endl;
		return -1;
	}
}


//д�ļ�������ʵ��д����ֽ���������2
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
				cout << "�޷�д��......" << endl;
				return 0;
			}
			if (inodes[index - 1].addr[k] == -1)
			{
				inodes[index - 1].addr[k] = AllocMemory();
				if (inodes[index - 1].addr[k] == -1)
				{
					cout << "�ռ䲻��,�޷�д������......" << endl;
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
							cout << "�ռ䲻�㣬д�벿������......" << endl;
							return (length - left);
						}

					}
				}
				if (left == 0)		//����ȫ��д���ļ�
				{
					cout << "д��ɹ�......" << endl;
					uot.offset[a] += length;
					return length;
				}
				else if (k >= 5)	//�����ļ�β����ֻд���˲���
				{
					cout << "д��ɹ�,д�벿������......" << endl;
					uot.offset[a] += (length - left);
					return (length - left);
				}
			}
		}
		else
		{
			cout << "Υ����Ȩ��......" << endl;
			return -1;
		}
	}
	else
	{
		cout << "�ļ�δ��......" << endl;
		return -1;
	}
}

//�ƶ��ļ�ָ�룬����1
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
			cout << "�ļ�δ��,�޷��ƶ��ļ�ָ��......" << endl;
	}
	else
		cout << "�ļ�������......" << endl;
}


//�ƶ��ļ�ָ�룬����2
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
		cout << "�ļ�δ��,�޷��ƶ��ļ�ָ��......" << endl;
		return 0;
	}
}

//��ȡ�ļ�ָ��λ�ã�����1
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

//��ȡ�ļ�ָ�룬����2
int getPos(int index)
{
	for (int i = 0; i < uot.numOfOpenedFile; i++)
		if (uot.list[i] == index)
			return uot.offset[i];
	return -1;
}


//�ر��ļ�
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

//��Ŀ¼
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
		cout << "  -" << (inodes[index - 1].file_type==0?"��ͨ�ļ�":"Ŀ¼�ļ�");
		cout << endl;
	}
}


//�û�����������򷵻ؽ��
void InputCommand()
{
	bool flag = false;
	cout << "******************************************" << endl;
	cout << "�ļ�ϵͳ������,��Q�˳�......" << endl;
	cout << "1.�����ļ�" << endl;
	cout << "2.���ļ�" << endl;
	cout << "3.��ȡ�ļ�" << endl;
	cout << "4.д���ļ�" << endl;
	cout << "5.ɾ���ļ�" << endl;
	cout << "6.�г���Ŀ¼�������ļ���Ŀ¼" << endl;
	cout << "7.��ʽ���ļ���" << endl;
	cout << "8.����(����ָ����ʾ)" << endl;
	cout << "******************************************" << endl;
	while (1)
	{
		if (flag)
		{
			cout << "******************************************" << endl;
			cout << "�ļ�ϵͳ������,��Q�˳�......" << endl;
			cout << "1.�����ļ�" << endl;
			cout << "2.���ļ�" << endl;
			cout << "3.��ȡ�ļ�" << endl;
			cout << "4.д���ļ�" << endl;
			cout << "5.ɾ���ļ�" << endl;
			cout << "6.�г���Ŀ¼�������ļ���Ŀ¼" << endl;
			cout << "7.��ʽ���ļ���" << endl;
			cout << "8.����(����ָ����ʾ)" << endl;
			cout << "******************************************" << endl;
		}
		cout << "��������ִ�е�ָ�" << endl;
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
		case '1':		//�����ļ�
			flag = false;
			cout << "�����ļ���(��ʽΪroot/xxx/xxx,rootΪ��Ŀ¼)��";
			cin >> filename;
			cout << "�����ļ�����(Ŀ¼�ļ�1������ͨ�ļ�0):";
			cin >> filetype;
			cout << "�����ļ���д��ʽ(ֻ��0��ֻд1���ɶ�д2):";
			cin >> mode;
		    index = createFile(filename, filetype, mode);
			if (index != -1)
			{
				cout << filename << "�����ɹ���" << endl;
				cout << "�ļ����fdΪ��" << index << endl;
			}
			else
				cout << "�����ļ�ʧ�ܣ�" << endl;
			break;
		case '2':		//���ļ�
			flag = false;

			cout << "�����ļ���(��ʽΪroot/xxx/xxx,rootΪ��Ŀ¼):";
			cin >> filename;
			cout << "����򿪷�ʽ(ֻ��0��ֻд1���ɶ�д2):";
			cin >> mode;
			index = openFile(filename, mode);
			if (index != -1)
			{
				cout << filename << "�Ѵ򿪣��ɽ���";
				if (mode == read_only)
					cout << "������" << endl;
				else if (mode == write_only)
					cout << "д����" << endl;
				else
					cout << "��д����" << endl;
				cout << "�ļ����fdΪ��" << index << endl;
			}
			else
				cout << "Υ����Ȩ�޻����ļ�������" << endl;
			break;
		case '3':		//���ļ�
			flag = false;
			cout << "�����ļ����:";
			cin >> fd;
			cout << "�ļ�ָ��ƫ��:";
			cin >> offset;
			cout << "�����ȡ���ֽ���:";
			cin >> length;
			if (length <= 0)
				cout << "��Ч�Ķ�����" << endl;
			else
			{
				buffer = new char[length+1];	//�����û���ȡ���ֽ�������ռ�
				memset(buffer, 0, length + 1);
				seekPos(fd, offset);
				read_len = readFile(fd, buffer, length);
				if (read_len > 0)
				{
					cout << "��ȡ��" << read_len << "�ֽ�......" << endl;
					cout << "�ļ�ָ�뵱ǰλ��:" << getPos(fd) << endl;
					char ch;
					cout << "�Ƿ���ʾ��ȡ����(Y/N):";
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
		case '4':		//д�ļ�
			flag = false;
			cout << "�����ļ����:";
			cin >> fd;
			cout << "�ļ�ָ��ƫ��:";
			cin >> offset;
			cout << "�Ƿ�д�볤��Ϊ800�ֽڵĲ�������(Y/N):";
			cin >> is800;
			if (is800 == 'Y' || is800 == 'y')
			{
				seekPos(fd, offset);
				write_len = writeFile(fd, test, 800);
				if (write_len == 800)
				{
					cout << "д��������ݳɹ�" << endl;
					cout << "�ļ�ָ�뵱ǰλ��:" << getPos(fd) << endl;
				}
			}
			else
			{
				cout << "������д�������:";
				cin >> write_content;
				length = write_content.length();
				if (length <= 0)
					cout << "��Ч�Ĳ���" << endl;
				else
				{
					seekPos(fd, offset);
					write_len = writeFile(fd, write_content.c_str(), length);
					if (write_len > 0)
					{
						cout << "д����" << write_len << "�ֽ�......" << endl;
						cout << "�ļ�ָ�뵱ǰλ��:" << getPos(fd) << endl;
					}
				}
			}	
			break;
		case '5':		//ɾ���ļ�
			flag = false;
			cout << "�����ļ���(��ʽΪroot/xxx/xxx,rootΪ��Ŀ¼):";
			cin >> filename;
			if (deleteFile(filename) == true)
				cout << "ɾ��" << filename << "�ɹ�" << endl;
			break;
		case '6':
			ls();
			flag = false;
			break;
		case '7':
			fformat();
			cout << "��ʽ���ļ���ɹ�" << endl;
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
			cout << "���Ϸ�������......" << endl;
			break;
		}
	}
}

//ÿһ���û��������֮�󣬱���
void Save()
{
	Disk.seekp(0);
	Disk.write((const char*)(&spblc), sizeof(spblc));
	for (int i = 0; i < numOfInode; ++i)
		Disk.write((const char*)(&inodes[i]), sizeof(INode));
	for (int i = 0; i < numOfDisknode; ++i)
		Disk.write((const char*)(&disknodes[i]), sizeof(DiskNode));
}


//������
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
			cout << "�ļ������ʧ��!" << endl;
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
			cout << "�ļ������ʧ��!" << endl;
			return -1;
		}
		InputCommand();
	}
	Save();
	Disk.close();
	return 0;
}