// Written by Fabian "ryg" Giesen.
// I hereby place this code in the public domain.

int instructionCount(unsigned char *func)
{
	int count = 0;

	while(*func != 0xCC)
	{
		// Skip prefixes F0h, F2h, F3h, 66h, 67h, D8h-DFh
		int operandSize = 4;
		int FPU = 0;
		while(*func == 0xF0 ||
		      *func == 0xF2 ||
		      *func == 0xF3 ||
		      (*func & 0xFE) == 0x66 ||
		      (*func & 0xF8) == 0xD8)
		{
			if(*func == 0x66)
			{
				operandSize = 2;
			}
			else if((*func & 0xF8) == 0xD8)
			{
				FPU = *func++;
				break;
			}

			func++;
		}

		// Skip two-byte opcode byte
		bool twoByte = false;
		if(*func == 0x0F)
		{
			twoByte = true;
			func++;
		}

		// Skip opcode byte
		unsigned char opcode = *func++;

		// Skip mod R/M byte
		unsigned char modRM = 0xFF;
		if(FPU)
		{
			if((opcode & 0xC0) != 0xC0)
			{
				modRM = opcode;
			}
		}
		else if(!twoByte)
		{
			if((opcode & 0xC4) == 0x00 ||
			   (opcode & 0xF4) == 0x60 && ((opcode & 0x0A) == 0x02 || (opcode & 0x09) == 0x9) ||
			   (opcode & 0xF0) == 0x80 ||
			   (opcode & 0xF8) == 0xC0 && (opcode & 0x0E) != 0x02 ||
			   (opcode & 0xFC) == 0xD0 ||
			   (opcode & 0xF6) == 0xF6)
			{
				modRM = *func++;
			}
		}
		else
		{
			if((opcode & 0xF0) == 0x00 && (opcode & 0x0F) >= 0x04 && (opcode & 0x0D) != 0x0D ||
			   (opcode & 0xF0) == 0x30 ||
			   opcode == 0x77 ||
			   (opcode & 0xF0) == 0x80 ||
			   (opcode & 0xF0) == 0xA0 && (opcode & 0x07) <= 0x02 ||
			   (opcode & 0xF8) == 0xC8)
			{
				// No mod R/M byte
			}
			else
			{
				modRM = *func++;
			}
		}

		// Skip SIB and displacement
		if((modRM & 0x07) == 0x04) func += 1;	// SIB
		if((modRM & 0xC5) == 0x05) func += 4;	// Dword displacement, no base
		if((modRM & 0xC0) == 0x40) func += 1;	// Byte displacement
		if((modRM & 0xC0) == 0x80) func += 4;	// Dword displacement

		// Skip immediate
		if(FPU)
		{
			// Can't have immediate operand
		}
		else if(!twoByte)
		{
			if((opcode & 0xC7) == 0x04 ||
			   (opcode & 0xFE) == 0x6A ||	// PUSH/POP/IMUL
			   (opcode & 0xF0) == 0x70 ||	// Jcc
			   opcode == 0x80 ||
			   opcode == 0x83 ||
			   (opcode & 0xFD) == 0xA0 ||	// MOV
			   opcode == 0xA8 ||		// TEST
			   opcode == 0xB0 ||		// MOV
			   (opcode & 0xFE) == 0xC0 ||	// RCL
			   opcode == 0xC6 ||		// MOV
			   opcode == 0xCD ||		// INT
			   (opcode & 0xFE) == 0xD4 ||	// AAD/AAM
			   (opcode & 0xF8) == 0xE0 ||	// LOOP/JCXZ
			   opcode == 0xEB ||
			   opcode == 0xF6 && (modRM & 0x30) == 0x00)	// TEST
			{
				func += 1;
			}
			else if((opcode & 0xF7) == 0xC2)
			{
				func += 2;   // RET
			}
			else if((opcode & 0xFC) == 0x80 ||
			        (opcode & 0xC7) == 0x05 ||
			        (opcode & 0xFE) == 0xE8 ||		// CALL/Jcc
					(opcode & 0xFE) == 0x68 ||
					(opcode & 0xFC) == 0xA0 ||
					(opcode & 0xEE) == 0xA8 ||
			        opcode == 0xC7 ||
			        opcode == 0xF7 && (modRM & 0x30) == 0x00)
			{
				func += operandSize;
			}
		}
		else
		{
			if(opcode == 0xBA ||		// BT
			   opcode == 0x0F ||		// 3DNow!
			   (opcode & 0xFC) == 0x70 ||	// PSLLW
			   (opcode & 0xF7) == 0xA4 ||	// SHLD
			   opcode == 0xC2 ||
			   opcode == 0xC4 ||
			   opcode == 0xC5 ||
			   opcode == 0xC6)
			{
				func += 1;
			}
			else if((opcode & 0xF0) == 0x80)
			{
				func += operandSize;   // Jcc -i
			}
		}

		count++;
	}

	return count;
}
 

