#pragma once
void NetworkInit();


// ���
struct stHEADER
{
	unsigned short Len;
};

// ��Ŷ
struct st_DRAW_PACKET
{
	int		iStartX;
	int		iStartY;
	int		iEndX;
	int		iEndY;
};
