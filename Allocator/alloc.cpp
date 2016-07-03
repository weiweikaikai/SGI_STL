/*************************************************************************
	> File Name: alloc.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 02 Jul 2016 08:31:37 PM CST
 ************************************************************************/

#include<iostream>
#include<stdlib.h>
#include<string.h>
#include<stdio.h>
using namespace std;
 
template<class T,class Alloc>
class SimpleAlloc
{
	public:
		static T* Allocate(size_t n)
	    {
	       return 0 == n ? 0:(T*)Alloc::Allocate(n*sizeof(T));
	    }
		static T* Allocate(void)
	   {
		   return (T*)Alloc::Allocate(sizeof(T));
	   }

	   static void Deallocate(T*p,size_t n)
	   {
	      if(0 != n)
		   {
		      Alloc::Deallocate(p,n*sizeof(T));
		   }
	   }
	   static void Deallocate(T *p)
	  {
	     Alloc::Deallocate(p,sizeof(T));
	   }
};


//一级空间配置器
typedef void (*ALLOC_OOM_FUN)();
template <int inst>
class _MallocAllocTemplate
{
	private:
		static ALLOC_OOM_FUN MallocOomHandler;

	    static void* OomMalloc(size_t n)
	     {
		    ALLOC_OOM_FUN handler;

			void *result;
			/*
			1.分配内存成功，则直接返回
			2.若分配失败，则检查是否设置处理的handler
			  有处理函数则调用以后再分配，不断重复这个过程，直到分配成功为止
			  没有设置处理handler 则直接结束程序
			*/

			for(;;)
			 {
			   handler = MallocOomHandler;
			   if(0 == handler)
				 {
			      cerr<<"out of memory"<<endl;
				  exit(-1);
			     }
				 (*handler)();
				 result = malloc(n);
				 if(result)
				 {
				   return result;
				 }
			}
		 }
	  static void* OomRealloc(void*p,size_t n)
	   {
	       ALLOC_OOM_FUN handler;
		   void *result;

		   for(;;)
		   {
		     handler=MallocOomHandler;
			  if(0 == handler)
				 {
			      cerr<<"out of memory"<<endl;
				  exit(-1);
			     }
				 (*handler)();
				 result = realloc(p,n);
				 if(result)
				 {
				   return result;
				 }
		   }

	   }
	public:
		static void* Allocate(size_t n)
	    {
	         void *result = malloc(n);
			 if(0 == result)
			  {
			    result = OomMalloc(n);
			  }
			  return result;
	    }
		static void Deallocate(void *p,size_t /*n*/)
	    {
		   free(p);
		}
		static void* Reallocate(void *p,size_t /*n*/,size_t new_sz)
	    {
		   void *result = realloc(p,new_sz);
		   if(0 == result)
			{
		      result=OomRealloc(p,new_sz);
		    }
			return result;
		}

};


template<int inst>
ALLOC_OOM_FUN _MallocAllocTemplate<inst>::MallocOomHandler=0;
typedef _MallocAllocTemplate<0> MallocAlloc;


template<bool threads,int inst>
class _DefaultAllocTemplate
{
	public:
		enum{_ALIGN=8};//排列基准值
	    enum{_MAX_BYTES = 128}; //最大值
		enum{_NFREELISTS=_MAX_BYTES/_ALIGN};//排列链大小
        
		static size_t Round_up(size_t bytes)
	    {
		  //对齐
		  return ((bytes +_ALIGN-1 ) &~(_ALIGN-1));
		}
        static size_t FreeList_Index(size_t bytes)
	     {
	      	return ((bytes+_ALIGN-1)/_ALIGN -1);
		 }
		 union Obj
	      {
		    union Obj* freelistlink;//指向下一个内存块的指针
			char clientData[1];    //客户端数据
		  };
		  static Obj* volatile freelist[_NFREELISTS]; //自由链表
		  static char* startfree;                     //内存池水位线开始
		  static char* endfree;                       //内存池水位线结束
		  static size_t heapSize;                     //从系统堆中分配的总大小

		  //获取大块内存插入自由链表中
		  static void* Refill(size_t n);
		  //从内存池中分配大块内存
		  static char* ChunkAlloc(size_t size,int &nobjs);

		  static void* Allocate(size_t n);
		  static void Deallocate(void*p,size_t n);
		  static void* Reallocate(void*p,size_t old_sz,size_t new_sz);
}; 

 //初始化全局静态对象
template<bool threads,int inst>
typename  _DefaultAllocTemplate<threads,inst>::Obj*volatile _DefaultAllocTemplate<threads,inst>::freelist[_DefaultAllocTemplate<threads,inst>::_NFREELISTS];
template <bool threads, int inst>
char* _DefaultAllocTemplate<threads, inst>::startfree = 0;
template <bool threads, int inst>
char* _DefaultAllocTemplate<threads, inst>::endfree = 0;
template <bool threads, int inst>
size_t _DefaultAllocTemplate<threads, inst>::heapSize = 0;


template<bool threads,int inst>
void * _DefaultAllocTemplate<threads,inst>::Refill(size_t n)
{
     //分配 20个n  bytes 的内存
	 //如果分配不够则能分配多少就分配多少
     printf("using Alloc Refill\n");
	 int nobjs = 20;
	 char * chunk = ChunkAlloc(n,nobjs);
   //如果只分配到一块，则直接用这块内存
	 if(nobjs == 1)
	  {
	  return chunk;
	  } 

	  Obj* result ;
	  Obj* cur;
	  size_t index = FreeList_Index(n);
	  result = (Obj*)chunk;  //把用户使用的空间留给用户
	  //把剩余的块链接到自由链表上面
	  cur =(Obj*)(chunk+n); 
	  freelist[index] = cur;

	  for(int i=2;i<nobjs;++i)
	   {
	     cur->freelistlink = (Obj*)(chunk+n*i); //强行将下一个内存块的地址转化为Obj类型的方便管理
		 cur = cur->freelistlink;  //尾插
	    }
		cur->freelistlink=NULL;
		return result;
}

template <bool threads, int inst>
char* _DefaultAllocTemplate<threads, inst>::ChunkAlloc(size_t size, int &nobjs)
{
    char *result;
	size_t bytesNeed = size*nobjs; //nobjs个size大小的内存 20*8 = 16   20*128=2560
	size_t bytesLeft = endfree-startfree;

    printf("using Alloc ChunkAlloc\n");
	/*
      1.内存池中的内存足够，bytesleft >= bytesNeed 则直接从内存中取
	  2.内存池中的内存不足，但是够一个bytesleft >=size  则直接取
	  3.内存池中的内存不足，则从系统堆分配大块内存到内存池中
	*/

    //能够从内存池中分配出20块出来就改变startfree
	if(bytesLeft >= bytesNeed)
	{
		printf("heapsize: %u  bytes :%u big need\n",heapSize,bytesLeft);
		//内存足够分配nobjs个对象
	    result = startfree;
		startfree += bytesNeed; //可以使用的空闲内存区
	}
	else if(bytesLeft >= size)  //不足以分配20块就分配一块
	{
		printf("heapsize: %u bytes :%d small need\n",heapSize,bytesLeft);
		//内存不足分配nobjs个对象
	   result = startfree;
	   nobjs = bytesLeft /size; //可能内存不足会导致nobjs比较小
	   startfree += nobjs *size;
	}
	else
	{
	    //再试一下若内存池中还有小块剩余内存
		//将他头插到合适的自由链表中
		if(bytesLeft > 0)
		{
		  size_t index = FreeList_Index(bytesLeft);
		  ((Obj*)startfree)->freelistlink= freelist[index];
		  freelist[index]=(Obj*)startfree;
		  startfree=NULL;
		}

		//从系统堆分配两倍+已分配的heapSize/8 的内存到内存池中
		size_t bytesToGet = 2*bytesNeed + Round_up(heapSize>>4); //2560*2 + 0>>4 =5120
		//内存池内存不足，系统堆重新分配
		startfree = (char*)malloc(bytesToGet);

        //无奈之举
		//如果在系统堆中内存分配失败，
		//则尝试到自由链表中更大的节点中分配

		if(startfree == NULL)
		{
		   //系统堆中没有足够，无奈只能到自由链表中看看
		   for(int i=size;i<_MAX_BYTES;i+=_ALIGN)
			{
		       Obj* head = freelist[FreeList_Index(size)];
			   if(head)
				{
			      startfree = (char*)head;
                  freelist[FreeList_Index(size)]=head->freelistlink;
				  endfree = startfree+i;
				  return ChunkAlloc(size,nobjs);
			   }
		     }
			 //自由链表中也没有分配到内存，则再到一级配置器中分配内存
			 //一级配置器中可能有设置的处理内存，或许能分配到内存
			 startfree=(char*)MallocAlloc::Allocate(bytesToGet);
		}

		//从系统堆中分配的总字节数（可用于下次分配时进行调节）
		heapSize += bytesToGet;
		endfree = startfree + bytesToGet;

		//递归调用获取内存
		 return ChunkAlloc(size,nobjs);
	}
	return result;
}

template <bool threads, int inst>
void* _DefaultAllocTemplate<threads, inst>::Allocate(size_t n)
{
   //若n > _MAX_BYTES则直接在一级配置器中获取
   //否则在二级配置器中获取
    printf("using Alloc Allocate\n");
     if(n >_MAX_BYTES)
	 {
	    return MallocAlloc::Allocate(n);
	 }
	 size_t index = FreeList_Index(n);  //(128+7)/8 -1 = 16-1=15   ---> 16*8 = 128

	 /*
	 1.如果自由链表中没有内存则通过Refill进行填充
	 2.如果自由链表中有责直接返回一个节点块内存
	 3.多线程环境需要考虑加锁
	 */
	 Obj* head = freelist[index];
	 if(head == NULL)
	  {
	    return Refill(Round_up(n)); //调整为8字节内存对齐再填充内存池
	  }
	  else  
	  {
	    //自由链表获取内存  //多线程环境需要考虑加锁
		freelist[index] = head->freelistlink;//相当于头删
		return head;
	  }
}
template <bool threads, int inst>
void _DefaultAllocTemplate<threads, inst>::Deallocate(void *p, size_t n)
{

	 printf("using Alloc Deallocate\n");
   //若 n> _MAX_BYTES 则直接归还给一级配置器
   //否则放回二级配置器的自由链表

    if(n>_MAX_BYTES)
	{
	  MallocAlloc::Deallocate(p,n);
	}
	else
	{
	    //多线程环境需要考虑加锁
		size_t index = FreeList_Index(n);

		//头插回自由链表
		Obj* tmp = (Obj*)p;
		tmp->freelistlink = freelist[index];
		freelist[index]=tmp;
	}
}

template <bool threads, int inst>
void* _DefaultAllocTemplate<threads, inst>::Reallocate(void *p, size_t old_sz,size_t new_sz)
{
        printf("using Alloc reallocate\n");

       void *result;
	   size_t copy_sz;

	   if(old_sz > (size_t)_MAX_BYTES && new_sz > (size_t)_MAX_BYTES)
	    {
	     return (realloc(p,new_sz));
	    }
		if(Round_up(old_sz) == Round_up(new_sz))
	    {
		  return p;
		}
		result = Allocate(new_sz);
		copy_sz  = new_sz > old_sz ? old_sz :new_sz;
         memcpy(result,p,copy_sz);
		 Deallocate(p,old_sz);
          
		  return result;
}
typedef _DefaultAllocTemplate<false, 0> Alloc;

void test1()
{
     cout<<"1."<<endl;

	 char *p1 =SimpleAlloc<char,MallocAlloc>::Allocate(129);
     SimpleAlloc<char,MallocAlloc>::Deallocate(p1,129);

	 cout<<"2."<<endl;
	 char *p2 =SimpleAlloc<char,Alloc>::Allocate(128);
	 char *p3 =SimpleAlloc<char,Alloc>::Allocate(128);
     SimpleAlloc<char,Alloc>::Deallocate(p2,128); 
	 SimpleAlloc<char,Alloc>::Deallocate(p3,128); 


    for(int i=0;i<41;++i)
	{
	   printf("test num: %d\n",i+1);
	  char *p=SimpleAlloc<char,Alloc>::Allocate(128);
	}
}
void test2()
{
   char *p1 = SimpleAlloc<char,Alloc>::Allocate(8);//320
   printf("\n");
   char *p2 = SimpleAlloc<char,Alloc>::Allocate(16);
    printf("\n");
   char *p3 = SimpleAlloc<char,Alloc>::Allocate(24);
    printf("\n");
   char *p4 = SimpleAlloc<char,Alloc>::Allocate(32);
    printf("\n");
   char *p5 = SimpleAlloc<char,Alloc>::Allocate(40);
    printf("\n");
   char *p6 = SimpleAlloc<char,Alloc>::Allocate(48);
    printf("\n");
   char *p7 = SimpleAlloc<char,Alloc>::Allocate(56);
    printf("\n");
   char *p8 = SimpleAlloc<char,Alloc>::Allocate(64);
    printf("\n");
   char *p9 = SimpleAlloc<char,Alloc>::Allocate(72);
    printf("\n");
   char *p10 = SimpleAlloc<char,Alloc>::Allocate(80);
    printf("\n");
   char *p11= SimpleAlloc<char,Alloc>::Allocate(88);
    printf("\n");
   char *p12 = SimpleAlloc<char,Alloc>::Allocate(96);
    printf("\n");
   char *p13 = SimpleAlloc<char,Alloc>::Allocate(104);
    printf("\n");
   char *p14 = SimpleAlloc<char,Alloc>::Allocate(112);
    printf("\n");
   char *p15= SimpleAlloc<char,Alloc>::Allocate(120);
    printf("\n");
   char *p16 = SimpleAlloc<char,Alloc>::Allocate(128);
}
void test3()
{
      char *p = SimpleAlloc<char,Alloc>::Allocate();
}

int main()
{
	test2();
    return 0;
}

