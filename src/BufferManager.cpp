#include <stdio.h>
#include <string.h>

#include "../include/BufferManager.h"
#include "../include/Kernel.h"

BufferManager::BufferManager()
{
	// nothing to do here
}

BufferManager::~BufferManager()
{
	// nothing to do here
}

void BufferManager::Initialize()
{
	printf("[info] BufferManager Init Info: 初始化BufferManager...\n");
	int i;
	Buf *bp;

	this->bFreeList.b_forw = this->bFreeList.b_back = &(this->bFreeList);
	// this->bFreeList.av_forw = this->bFreeList.av_back = &(this->bFreeList);

	for (i = 0; i < NBUF; i++)
	{
		bp = &(this->m_Buf[i]);
		// bp->b_dev = -1;
		bp->b_addr = this->Buffer[i];
		/* 初始化NODEV队列 */
		bp->b_back = &(this->bFreeList);
		bp->b_forw = this->bFreeList.b_forw;
		this->bFreeList.b_forw->b_back = bp;
		this->bFreeList.b_forw = bp;
		/* 初始化自由队列 */
		pthread_mutex_init(&bp->buf_lock, NULL);
		pthread_mutex_lock(&bp->buf_lock);
		bp->b_flags = Buf::B_BUSY;
		Brelse(bp);
	}
	return;
}

// TODO: 非常不同
Buf *BufferManager::GetBlk(int blkno)
{
	Buf *headbp = &(this->bFreeList);
	Buf *bp;
	// 查看bFreeList中是否已经有该块的缓存, 有就返回
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		if (bp->b_blkno != blkno)
			continue;
		bp->b_flags |= Buf::B_BUSY;
		pthread_mutex_lock(&bp->buf_lock);
		return bp;
	}

	// 没找到就去队头找
	int success = false;
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		// 检查该buf是否上锁
		if (pthread_mutex_trylock(&bp->buf_lock) == 0)
		{
			success = true;
			break;
		}
		printf("[debug] buf已被锁，blkno=%d b_addr=%p\n", bp->b_blkno, bp->b_addr);
	}
	if (success == false)
	{
		bp = headbp->b_forw;
		printf("[INFO]系统缓存已用完，等待队首缓存解锁...\n");
		pthread_mutex_lock(&bp->buf_lock); // 等待第一个缓存块解锁。
		printf("[INFO]系统成功得到队首缓存块...\n");
	}
	if (bp->b_flags & Buf::B_DELWRI)
	{
		// 理应延迟写，但这里无需考虑上锁等操作，故直接写
		this->Bwrite(bp);
		pthread_mutex_lock(&bp->buf_lock); // 马上上锁
	}

	// 已写回，清空其他标志
	bp->b_flags = Buf::B_BUSY;

	// 由于只有一个dev，故直接将头节点变为尾节点
	// // 将bp从原位置移出
	// bp->b_back->b_forw = bp->b_forw;
	// bp->b_forw->b_back = bp->b_back;
	// // 移到尾节点
	// bp->b_back = this->bFreeList.b_back->b_forw;
	// this->bFreeList.b_back->b_forw = bp;
	// bp->b_forw = &(this->bFreeList);
	// this->bFreeList.b_back = bp;

	bp->b_blkno = blkno;
	return bp;
}

void BufferManager::Brelse(Buf *bp)
{
	/* 注意以下操作并没有清除B_DELWRI、B_WRITE、B_READ、B_DONE标志
	 * B_DELWRI表示虽然将该控制块释放到自由队列里面，但是有可能还没有些到磁盘上。
	 * B_DONE则是指该缓存的内容正确地反映了存储在或应存储在磁盘上的信息
	 */
	bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);
	pthread_mutex_unlock(&bp->buf_lock);
	return;
}

Buf *BufferManager::Bread(int blkno)
{
	Buf *bp;
	/* 根据字符块号申请缓存 */
	bp = this->GetBlk(blkno);
	/* 如果在设备队列中找到所需缓存，即B_DONE已设置，就不需进行I/O操作 */
	if (bp->b_flags & Buf::B_DONE)
	{
		return bp;
	}
	/* 没有找到相应缓存，构成I/O读请求块 */
	bp->b_flags |= Buf::B_READ;
	bp->b_wcount = BufferManager::BUFFER_SIZE;

	/*
	 * 将I/O请求块送入相应设备I/O请求队列，如无其它I/O请求，则将立即执行本次I/O请求；
	 * 否则等待当前I/O请求执行完毕后，由中断处理程序启动执行此请求。
	 * 注：Strategy()函数将I/O请求块送入设备请求队列后，不等I/O操作执行完毕，就直接返回。
	 */

	// 这里直接拷贝
	memcpy(bp->b_addr, &this->p[BufferManager::BUFFER_SIZE * bp->b_blkno], bp->b_wcount);
	return bp;
}

void BufferManager::Bwrite(Buf *bp)
{
	bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
	bp->b_wcount = BufferManager::BUFFER_SIZE; /* 512字节 */

	memcpy(&this->p[BufferManager::BUFFER_SIZE * bp->b_blkno], bp->b_addr, bp->b_wcount);
	this->Brelse(bp);

	return;
}

void BufferManager::Bdwrite(Buf *bp)
{
	/* 置上B_DONE允许其它进程使用该磁盘块内容 */
	bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
	this->Brelse(bp);
	return;
}

void BufferManager::Bawrite(Buf *bp)
{
	/* 标记为异步写 */
	bp->b_flags |= Buf::B_ASYNC;
	this->Bwrite(bp);
	return;
}

void BufferManager::ClrBuf(Buf *bp)
{
	int *pInt = (int *)bp->b_addr;

	/* 将缓冲区中数据清零 */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
	{
		pInt[i] = 0;
	}
	return;
}

void BufferManager::Bflush()
{
	// printf("[info] BufferManager: Bflush\n");
	Buf *bp;
	/* 注意：这里之所以要在搜索到一个块之后重新开始搜索，
	 * 因为在bwite()进入到驱动程序中时有开中断的操作，所以
	 * 等到bwrite执行完成后，CPU已处于开中断状态，所以很
	 * 有可能在这期间产生磁盘中断，使得bfreelist队列出现变化，
	 * 如果这里继续往下搜索，而不是重新开始搜索那么很可能在
	 * 操作bfreelist队列的时候出现错误。
	 */
	for (bp = this->bFreeList.b_forw; bp != &(this->bFreeList); bp = bp->b_forw)
	{
		// printf("current: %ld\n", bp - &this->bFreeList);
		//	cout << "该缓存块指向哪一个磁盘" << bp->b_blkno << endl;
		/* 找出自由队列中所有延迟写的块 */
		if ((bp->b_flags & Buf::B_DELWRI))
		{
			// printf("selected blkno: %d\n", bp->b_blkno);
			//	cout << "找到延迟写的块" << endl;
			// 注：将它放在队列的尾部
			bp->b_back->b_forw = bp->b_forw;
			bp->b_forw->b_back = bp->b_back;

			bp->b_back = this->bFreeList.b_back->b_forw;
			this->bFreeList.b_back->b_forw = bp;
			bp->b_forw = &this->bFreeList;
			this->bFreeList.b_back = bp;
			// printf("can bwrite\n");
			this->Bwrite(bp);
		}
	}
	return;
}

void BufferManager::GetError(Buf *bp)
{
	User &u = Kernel::Instance().GetUser();

	if (bp->b_flags & Buf::B_ERROR)
	{
		u.u_error = EIO;
	}
	return;
}

Buf *BufferManager::InCore(int blkno)
{
	Buf *bp;
	Buf *headbp = &this->bFreeList;
	for (bp = headbp->b_forw; bp != headbp; bp = bp->b_forw)
	{
		if (bp->b_blkno == blkno)
			return bp;
	}
	return NULL;
}

Buf &BufferManager::GetBFreeList()
{
	return this->bFreeList;
}

void BufferManager::SetP(char *p_addr)
{
	this->p = p_addr;
}

char *BufferManager::GetP()
{
	return this->p;
}