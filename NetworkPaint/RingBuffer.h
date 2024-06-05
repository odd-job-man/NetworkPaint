#pragma once
#define IN
#define OUT

constexpr int BUFFER_SIZE = 10000;

class RingBuffer
{
public:
	RingBuffer(void);
	~RingBuffer(void);
	int GetUseSize(void);
	int GetFreeSize(void);
	int Enqueue(IN char* pSource, IN size_t sizeToPut);
	int Dequeue(OUT char* pDest, IN size_t sizeToRead);
	int Peek(IN int sizeToPeek, OUT char* pTarget);
	void ClearBuffer(void);
	int DirectEnqueueSize(void);
	int DirectDequeueSize(void);
	int MoveRear(IN int sizeToMove);
	int MoveFront(IN int sizeToMove);
	char* GetFrontPtr(void);
	char* GetRearPtr(void);
	char* GetWriteStartPtr(void);
	char* GetReadStartPtr(void);

	char* pBuffer_;
	int front_;
	int rear_;
};
