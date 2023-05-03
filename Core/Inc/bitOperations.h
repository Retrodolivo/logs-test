#ifndef _BIT_OPERATIONS_H_
#define BitCtr(reg, bitNamber) reg &= ~(1 << (bitNamber))			//�������� ��� �������� reg � "0"
#define BitSet(reg, bitNamber) reg |= (1 << (bitNamber))			//���������� ��� �������� reg � "1"
#define BitStatus(reg, bitNamber) ((reg) & (1 << (bitNamber)))		//�������� ���� �������� reg
#define TRUE 1														//������
#define FALSE (!TRUE)												//����
#endif
