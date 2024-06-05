#include "RingBuffer.h"
#include <math.h>
#include <memory.h>

// 특이사항 : 원형 큐이기 때문에 할당 크기가 BUFFER_SIZE 보다 1 커야함
RingBuffer::RingBuffer(void)
{
	pBuffer_ = new char[BUFFER_SIZE + 1];
	front_ = rear_ = 0;
}

RingBuffer::~RingBuffer(void)
{
	delete[] pBuffer_;
}

// Return:  (int) 현재 Buffer에서 Enqueue되어있는 크기
int RingBuffer::GetUseSize(void)
{
	if (rear_ - front_ >= 0)
	{
		return rear_ - front_;
	}
	else
	{
		return BUFFER_SIZE - (front_ - rear_ - 1);
	}
}

// Return:(int)현재 Buffer에 추가로 Enequeue 가능한 크기
int RingBuffer::GetFreeSize(void)
{
	return BUFFER_SIZE - GetUseSize();
}

//--------------------------------------------------------------------
// 기능: Ring Buffer에 Data 삽입
// Parameters:	IN (char *) Ring Buffer에 Data를 넣을 대상 Buffer
//				IN (int) 복사할 크기
// Return:		(int) Ring Buffer에 들어간 크기(사실상 0 혹은 sizeToPut 둘중 하나다)
// 특이사항 : Enqueue정책은 sizeToPut() > GetFreeSize()이면 0을 반환한다.
//--------------------------------------------------------------------
int RingBuffer::Enqueue(IN char* pDest, IN size_t sizeToPut)
{
	int freeSize = GetFreeSize();	
	if (sizeToPut > freeSize)
	{
		// 반환하는 쪽에서는 연결을 끊어버려야함.
		return 0;
	}
	int DirectSize = DirectEnqueueSize();

	// DirectSize가 sizeToPut보다 크면 
	int firstSize = DirectSize > sizeToPut ? sizeToPut : DirectSize;
	int secondSize = sizeToPut - firstSize;

	
	memcpy_s(GetWriteStartPtr(), firstSize, pDest, firstSize);
	MoveRear(firstSize);
	//memcpy_s(pBuffer_ + rear_ + 1, firstSize, pDest, firstSize);
	//rear_ = (rear_ + firstSize) % (BUFFER_SIZE + 1);
	if (secondSize <= 0)
	{
		return firstSize;
	}
	memcpy_s(GetWriteStartPtr(), secondSize, pDest + firstSize, secondSize);
	MoveRear(secondSize);
	//memcpy_s(pBuffer_, secondSize, pDest + firstSize, secondSize);
	//rear_ = (rear_ + secondSize) % (BUFFER_SIZE + 1);
	return firstSize + secondSize;
}

//--------------------------------------------------------------------
// 기능: Ring Buffer에서 Data 꺼냄
// Parameters:	OUT (char *) Ring Buffer의 Data를 받을 대상 Buffer
//				IN (int) 복사할 크기
// Return:		(int) Ring Buffer에서 꺼내서 pDest에 복사한 데이터의 크기
// 특이사항 : Dequeue정책은 sizeToRead > GetUseSize()이면 Dequeue를 제대로 수행하지않고 0을 반환한다.
//--------------------------------------------------------------------
int RingBuffer::Dequeue(OUT char* pDest, IN size_t sizeToRead)
{
	int useSize = GetUseSize();
	if (sizeToRead > useSize)
	{
		// 들어있는 데이터보다 많은 데이터를 읽으려고 하면 그냥 반환한다.
		return 0;
	}
	int directSize = DirectDequeueSize();
	int firstSize = directSize > sizeToRead ? sizeToRead : directSize;
	int secondSize = sizeToRead - firstSize;

	memcpy_s(pDest, firstSize, GetReadStartPtr(), firstSize);
	MoveFront(firstSize);
	//memcpy_s(pDest, firstSize, pBuffer_ + front_ + 1, firstSize);
	//front_ = (front_ + firstSize) % (BUFFER_SIZE + 1);
	if (secondSize <= 0)
		return firstSize;
	memcpy_s(pDest + firstSize, secondSize, GetReadStartPtr(), secondSize);
	MoveFront(secondSize);
	//memcpy_s(pDest + firstSize, secondSize, pBuffer_, secondSize);
	//front_ = (front_ + secondSize) % (BUFFER_SIZE + 1);
	return firstSize + secondSize;
}

// 해당함수는 Dequeue와 거의 같지만 복사만 수행하고 front의 위치가 바뀌지 않는다. 
int RingBuffer::Peek(IN int sizeToPeek, OUT char* pTarget)
{
	int useSize = GetUseSize();
	if (sizeToPeek > useSize)
	{
		// 들어있는 데이터보다 많은 데이터를 읽으려고 하면 그냥 반환한다.
		return 0;
	}
	int directSize = DirectDequeueSize();
	int firstSize = directSize > sizeToPeek ? sizeToPeek: directSize;
	int secondSize = sizeToPeek - firstSize;

	//memcpy_s(pTarget, firstSize, GetReadStartPtr(), firstSize);
	memcpy_s(pTarget, firstSize, pBuffer_ + front_ + 1, firstSize);
	if (secondSize <= 0)
		return firstSize;
	memcpy_s(pTarget + firstSize, secondSize, pBuffer_, secondSize);
	return firstSize + secondSize;
}

void RingBuffer::ClearBuffer(void)
{
	rear_ = front_;
}


// 기능 : rear_ + 1부터 큐의 맨앞으로 이동하지 않고 Enqueue가능한 최대 크기를 반환함.
// 즉 rear+ 1부터 front_ - 1까지의 거리 혹은 rear_ + 1부터 
int RingBuffer::DirectEnqueueSize(void)
{
	if (rear_ >= front_)
	{
		return BUFFER_SIZE - rear_;
	}
	else
	{
		return front_ - rear_ - 1;
	}
}

int RingBuffer::DirectDequeueSize(void)
{
	if (rear_ >= front_)
	{
		return rear_ - front_;
	}
	else
	{
		return BUFFER_SIZE  - front_;
	}
}

// sizeToMove만큼 rear_를 이동시키고 현재 rear_를 반환함.
int RingBuffer::MoveRear(IN int sizeToMove)
{
	rear_ = (rear_ + sizeToMove) % (BUFFER_SIZE + 1);
	return rear_;
}

// sizeToMove만큼 front_이동시키고 현재 rear_를 반환함.
int RingBuffer::MoveFront(IN int sizeToMove)
{
	front_ = (front_ + sizeToMove) % (BUFFER_SIZE + 1);
	return front_;
}

// 현재 front_인덱스에 알맞는 포인터를 반환
char* RingBuffer::GetFrontPtr(void)
{
	return pBuffer_ + front_;
}

// 현재 rear_ 인덱스에 알맞는 포인터를 반환
char* RingBuffer::GetRearPtr(void)
{
	return pBuffer_ + rear_;
}

char* RingBuffer::GetWriteStartPtr(void)
{
	return pBuffer_ + ((rear_ + 1) % (BUFFER_SIZE + 1));
}

char* RingBuffer::GetReadStartPtr(void)
{
	return pBuffer_ + ((front_ + 1) % (BUFFER_SIZE + 1));
}


