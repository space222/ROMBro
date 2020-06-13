#include <cstdio>
#include <cstdlib>
#include "types.h"

reg cpu[5];
u16 PC = 0;

#define A cpu[0].b.h
#define F cpu[0].b.l
#define B cpu[1].b.h
#define C cpu[1].b.l
#define D cpu[2].b.h
#define E cpu[2].b.l
#define H cpu[3].b.h
#define L cpu[3].b.l
#define AF cpu[0].w
#define BC cpu[1].w
#define DE cpu[2].w
#define HL cpu[3].w
#define SP cpu[4].w

#define FLAG_Z 0x80
#define FLAG_N 0x40
#define FLAG_H 0x20
#define FLAG_C 0x10

void mem_write8(u16, u8);
void mem_write16(u16 addr, u16 val)
{
	mem_write8(addr, val>>8);
	mem_write8(addr+1, val&0xFF);
	return;
}
u8 mem_read8(u16);
u8 io_read8(u16);
void io_write8(u16, u8);

void cb_prefix(u8 op);
void add(u8 p);
void adc(u8 p);
void add_hl(u16 p);
void daa();
void cp(u8 p);
void sub(u8 p);
void sbc(u8 p);
u16 imm16();
void push16(u16);
u16 pop16();

void gb_interpret()
{
	u8 op = mem_read8(PC++);
	u8 temp = 0;
	
	switch( op )
	{
	case 0x00: break; // nop
	case 0x01: BC = imm16(); break;
	case 0x02: mem_write8(BC, A); break;
	case 0x03: BC++; break;
	case 0x04: F &= ~(FLAG_Z|FLAG_N|FLAG_H); if( (B & 0xF) == 0xF ) F |= FLAG_H; B++; if( B == 0 ) F |= FLAG_Z; break;
	case 0x05: F |= FLAG_N; F &= ~(FLAG_Z|FLAG_H); if( B & 0xF ) F |= FLAG_H; B--; if( B == 0 ) F |= FLAG_Z; break;
	case 0x06: B = mem_read8(PC++); break; // LD B, n
	case 0x07: F = 0; if( A >> 7 ) F |= FLAG_C; A = (A<<1)|(A>>7); if( A == 0 ) F |= FLAG_Z; break;
	case 0x08: mem_write16(imm16(), SP); break;
	case 0x09: add_hl(BC); break;
	case 0x0A: A = mem_read8(BC);   break; // LD A, (BC)
	case 0x0B: BC--; break;
	case 0x0C: F &= ~(FLAG_Z|FLAG_N|FLAG_H); if( (C & 0xF) == 0xF ) F |= FLAG_H; C++; if( C == 0 ) F |= FLAG_Z; break;	
	case 0x0D: F |= FLAG_N; F &= ~(FLAG_Z|FLAG_H); if( C & 0xF ) F |= FLAG_H; C--; if( C == 0 ) F |= FLAG_Z; break;
	case 0x0E: C = mem_read8(PC++); break; // LD C, n
	case 0x0F: F = 0; if( A & 1 ) F |= FLAG_C; A = (A>>1)|(A<<7); if( A == 0 ) F |= FLAG_Z; break;
	case 0x10: break; //TODO: stop
	case 0x11: DE = imm16(); break;
	case 0x12: mem_write8(DE, A); break;
	case 0x13: DE++; break;
	case 0x14: F &= ~(FLAG_Z|FLAG_N|FLAG_H); if( (D & 0xF) == 0xF ) F |= FLAG_H; D++; if( D == 0 ) F |= FLAG_Z; break;
	case 0x15: F |= FLAG_N; F &= ~(FLAG_Z|FLAG_H); if( D & 0xF ) F |= FLAG_H; D--; if( D == 0 ) F |= FLAG_Z; break;	
	case 0x16: D = mem_read8(PC++); break; // LD D, n
	case 0x17: temp = (F>>4)&1; F = 0; if( A >> 7 ) F |= FLAG_C; A = (A<<1)|temp; if( A == 0 ) F |= FLAG_Z; break;
	case 0x18: temp = mem_read8(PC++); PC += (s8)temp; break;
	case 0x19: add_hl(DE); break;
	case 0x1A: A = mem_read8(DE);   break; // LD A, (DE)
	case 0x1B: DE--; break;
	case 0x1C: F &= ~(FLAG_Z|FLAG_N|FLAG_H); if( (E & 0xF) == 0xF ) F |= FLAG_H; E++; if( E == 0 ) F |= FLAG_Z; break;
	case 0x1D: F |= FLAG_N; F &= ~(FLAG_Z|FLAG_H); if( E & 0xF ) F |= FLAG_H; E--; if( E == 0 ) F |= FLAG_Z; break;	
	case 0x1E: E = mem_read8(PC++); break; // LD E, n
	case 0x1F: temp = (F>>4)&1; F = 0; if( A & 1 ) F |= FLAG_C; A = (A>>1)|(temp<<7); if( A == 0 ) F |= FLAG_Z; break;
	case 0x20: temp = mem_read8(PC++); if( !(F & FLAG_Z) ) PC += (s8)temp; break;
	case 0x21: HL = imm16(); break;
	case 0x22: mem_write8(HL++, A); break;
	case 0x23: HL++; break;
	case 0x24: F &= ~(FLAG_Z|FLAG_N|FLAG_H); if( (H & 0xF) == 0xF ) F |= FLAG_H; H++; if( H == 0 ) F |= FLAG_Z; break;
	case 0x25: F |= FLAG_N; F &= ~(FLAG_Z|FLAG_H); if( H & 0xF ) F |= FLAG_H; H--; if( H == 0 ) F |= FLAG_Z; break;	
	case 0x26: H = mem_read8(PC++); break; // LD H, n
	case 0x27: daa(); break;
	case 0x28: temp = mem_read8(PC++); if( F & FLAG_Z ) PC += (s8)temp; break;	
	case 0x29: add_hl(HL); break;
	case 0x2A: A = mem_read8(HL++); break;
	case 0x2B: HL--; break;
	case 0x2C: F &= ~(FLAG_Z|FLAG_N|FLAG_H); if( (L & 0xF) == 0xF ) F |= FLAG_H; L++; if( L == 0 ) F |= FLAG_Z; break;
	case 0x2D: F |= FLAG_N; F &= ~(FLAG_Z|FLAG_H); if( L & 0xF ) F |= FLAG_H; L--; if( L == 0 ) F |= FLAG_Z; break;
	case 0x2E: L = mem_read8(PC++); break; // LD L, n
	case 0x2F: A = ~A; F |= (FLAG_N|FLAG_H); break;
	case 0x30: temp = mem_read8(PC++); if( !(F & FLAG_C) ) PC += (s8)temp; break;	
	case 0x31: SP = imm16(); break;	
	case 0x32: mem_write8(HL--, A); break;
	case 0x33: SP++; break;
	case 0x34: 
		temp = mem_read8(HL);
		F &= ~(FLAG_Z|FLAG_N|FLAG_H); 
		if( (temp & 0xF) == 0xF ) F |= FLAG_H; 
		temp++; 
		if( temp == 0 ) F |= FLAG_Z;
		break;
	case 0x35:
		temp = mem_read8(HL); 
		F |= FLAG_N; 
		F &= ~(FLAG_Z|FLAG_H);
		if( temp & 0xF ) F |= FLAG_H; 
		temp--; 
		if( temp == 0 ) F |= FLAG_Z;
		mem_write8(HL, temp);
		break;
	case 0x36: mem_write8(HL, mem_read8(PC++)); break;
	case 0x37: F &= ~(FLAG_N|FLAG_H); F |= FLAG_C; break; // SCF
	case 0x38: temp = mem_read8(PC++); if( F & FLAG_C ) PC += (s16)(s8)temp; break;
	case 0x39: add_hl(SP); break;
	case 0x3A: A = mem_read8(HL--); break;
	case 0x3B: SP--; break;
	case 0x3C: F &= ~(FLAG_Z|FLAG_N|FLAG_H); if( (A & 0xF) == 0xF ) F |= FLAG_H; A++; if( A == 0 ) F |= FLAG_Z; break;
	case 0x3D: F |= FLAG_N; F &= ~(FLAG_Z|FLAG_H); if( A & 0xF ) F |= FLAG_H; A--; if( A == 0 ) F |= FLAG_Z; break;
	case 0x3E: A = mem_read8(PC++); break;
	case 0x3F: F &= ~(FLAG_N|FLAG_H); F ^= FLAG_C; break; // CCF
	case 0x40: B = B; break;
	case 0x41: B = C; break;
	case 0x42: B = D; break;
	case 0x43: B = E; break;
	case 0x44: B = H; break;
	case 0x45: B = L; break;
	case 0x46: B = mem_read8(HL); break;
	case 0x47: B = A; break;
	case 0x48: C = B; break;
	case 0x49: C = C; break;
	case 0x4A: C = D; break;
	case 0x4B: C = E; break;
	case 0x4C: C = H; break;
	case 0x4D: C = L; break;
	case 0x4E: C = mem_read8(HL); break;
	case 0x4F: C = A; break;
	case 0x50: D = B; break;
	case 0x51: D = C; break;
	case 0x52: D = D; break;
	case 0x53: D = E; break;
	case 0x54: D = H; break;
	case 0x55: D = L; break;
	case 0x56: D = mem_read8(HL); break;
	case 0x57: D = A; break;
	case 0x58: E = B; break;
	case 0x59: E = C; break;
	case 0x5A: E = D; break;
	case 0x5B: E = E; break;
	case 0x5C: E = H; break;
	case 0x5D: E = L; break;
	case 0x5E: E = mem_read8(HL); break;
	case 0x5F: E = A; break;
	case 0x60: H = B; break;
	case 0x61: H = C; break;
	case 0x62: H = D; break;
	case 0x63: H = E; break;
	case 0x64: H = H; break;
	case 0x65: H = L; break;
	case 0x66: H = mem_read8(HL); break;
	case 0x67: H = A; break;
	case 0x68: L = B; break;
	case 0x69: L = C; break;
	case 0x6A: L = D; break;
	case 0x6B: L = E; break;
	case 0x6C: L = H; break;
	case 0x6D: L = L; break;
	case 0x6E: L = mem_read8(HL); break;
	case 0x6F: L = A; break;
	case 0x70: mem_write8(HL, B); break;
	case 0x71: mem_write8(HL, C); break;
	case 0x72: mem_write8(HL, D); break;
	case 0x73: mem_write8(HL, E); break;
	case 0x74: mem_write8(HL, H); break;
	case 0x75: mem_write8(HL, L); break;
	case 0x76: break; //TODO: halt
	case 0x77: mem_write8(HL, A); break;
	case 0x78: A = B; break;
	case 0x79: A = C; break;
	case 0x7A: A = D; break;
	case 0x7B: A = E; break;
	case 0x7C: A = H; break;
	case 0x7D: A = L; break;
	case 0x7E: A = mem_read8(HL); break;
	case 0x7F: A = A; break;
	case 0x80: add(B); break;
	case 0x81: add(C); break;
	case 0x82: add(D); break;
	case 0x83: add(E); break;
	case 0x84: add(H); break;
	case 0x85: add(L); break;
	case 0x86: add(mem_read8(HL)); break;
	case 0x87: add(A); break;
	case 0x88: adc(B); break;
	case 0x89: adc(C); break;
	case 0x8A: adc(D); break;
	case 0x8B: adc(E); break;
	case 0x8C: adc(H); break;
	case 0x8D: adc(L); break;
	case 0x8E: adc(mem_read8(HL)); break;
	case 0x8F: adc(A); break;
	case 0x90: sub(B); break;
	case 0x91: sub(C); break;
	case 0x92: sub(D); break;
	case 0x93: sub(E); break;
	case 0x94: sub(H); break;
	case 0x95: sub(L); break;
	case 0x96: sub(mem_read8(HL)); break;
	case 0x97: sub(A); break;
	case 0x98: sbc(B); break;
	case 0x99: sbc(C); break;
	case 0x9A: sbc(D); break;
	case 0x9B: sbc(E); break;
	case 0x9C: sbc(H); break;
	case 0x9D: sbc(L); break;
	case 0x9E: sbc(mem_read8(HL)); break;
	case 0x9F: sbc(A); break;
	case 0xA0: F = FLAG_H; A &= B; if( A == 0 ) F |= FLAG_Z; break;
	case 0xA1: F = FLAG_H; A &= C; if( A == 0 ) F |= FLAG_Z; break;
	case 0xA2: F = FLAG_H; A &= D; if( A == 0 ) F |= FLAG_Z; break;
	case 0xA3: F = FLAG_H; A &= E; if( A == 0 ) F |= FLAG_Z; break;
	case 0xA4: F = FLAG_H; A &= H; if( A == 0 ) F |= FLAG_Z; break;
	case 0xA5: F = FLAG_H; A &= L; if( A == 0 ) F |= FLAG_Z; break;
	case 0xA6: F = FLAG_H; A &= mem_read8(HL); if( A == 0 ) F |= FLAG_Z; break;
	case 0xA7: F = FLAG_H; A &= A; if( A == 0 ) F |= FLAG_Z; break;	
	case 0xA8: F = 0; A ^= B; if( A == 0 ) F |= FLAG_Z; break;
	case 0xA9: F = 0; A ^= C; if( A == 0 ) F |= FLAG_Z; break;
	case 0xAA: F = 0; A ^= D; if( A == 0 ) F |= FLAG_Z; break;
	case 0xAB: F = 0; A ^= E; if( A == 0 ) F |= FLAG_Z; break;
	case 0xAC: F = 0; A ^= H; if( A == 0 ) F |= FLAG_Z; break;
	case 0xAD: F = 0; A ^= L; if( A == 0 ) F |= FLAG_Z; break;
	case 0xAE: F = 0; A ^= mem_read8(HL); if( A == 0 ) F |= FLAG_Z; break;
	case 0xAF: F = 0; A ^= A; if( A == 0 ) F |= FLAG_Z; break;
	case 0xB0: F = 0; A |= B; if( A == 0 ) F |= FLAG_Z; break;
	case 0xB1: F = 0; A |= C; if( A == 0 ) F |= FLAG_Z; break;
	case 0xB2: F = 0; A |= D; if( A == 0 ) F |= FLAG_Z; break;
	case 0xB3: F = 0; A |= E; if( A == 0 ) F |= FLAG_Z; break;
	case 0xB4: F = 0; A |= H; if( A == 0 ) F |= FLAG_Z; break;
	case 0xB5: F = 0; A |= L; if( A == 0 ) F |= FLAG_Z; break;
	case 0xB6: F = 0; A |= mem_read8(HL); if( A == 0 ) F |= FLAG_Z; break;
	case 0xB7: F = 0; A |= A; if( A == 0 ) F |= FLAG_Z; break;	
	case 0xB8: cp(B); break;
	case 0xB9: cp(C); break;
	case 0xBA: cp(D); break;
	case 0xBB: cp(E); break;
	case 0xBC: cp(H); break;
	case 0xBD: cp(L); break;
	case 0xBE: cp(mem_read8(HL)); break;
	case 0xBF: cp(A); break;	
	case 0xC0: if( !(F & FLAG_Z) ) PC = pop16(); break;
	case 0xC1: BC = pop16(); break;
	case 0xC2: if( !(F & FLAG_Z) ) PC = imm16(); else PC += 2; break;
	case 0xC3: PC = imm16(); break;
	case 0xC4: if( !(F & FLAG_Z) ) { push16(PC+2); PC = imm16(); } else PC += 2; break;
	case 0xC5: push16(BC); break;
	case 0xC6: add(mem_read8(PC++)); break;
	case 0xC7: push16(PC); PC = 0; break;
	case 0xC8: if( F & FLAG_Z ) PC = pop16(); break;
	case 0xC9: PC = pop16(); break;
	case 0xCA: if( F & FLAG_Z ) PC = imm16(); else PC += 2; break;
	case 0xCB: PC++; cb_prefix(mem_read8(PC)); break;
	case 0xCC: if( F & FLAG_Z ) { push16(PC+2); PC = imm16(); } else PC += 2; break;
	case 0xCD: push16(PC+2); PC = imm16(); break;
	case 0xCE: adc(mem_read8(PC++)); break;
	case 0xCF: push16(PC); PC = 8; break;
	case 0xD0: if( !(F & FLAG_C) ) PC = pop16(); break;
	case 0xD1: DE = pop16(); break;
	case 0xD2: if( !(F & FLAG_C) ) PC = imm16(); else PC += 2; break;
	//   0xD3 is undefined
	case 0xD4: if( !(F & FLAG_C) ) { push16(PC+2); PC = imm16(); } else PC += 2; break;
	case 0xD5: push16(DE); break;
	case 0xD6: sub(mem_read8(PC++)); break;
	case 0xD7: push16(PC); PC = 0x10; break;
	case 0xD8: if( F & FLAG_C ) PC = pop16(); break;
	case 0xD9: // RETI
		PC = imm16();
		//TODO: enable interrupts	
		break;
	case 0xDA: if( F & FLAG_C ) PC = imm16(); else PC += 2; break;
	//   0xDB is undefined
	case 0xDC: if( F & FLAG_C ) { push16(PC+2); PC = imm16(); } else PC += 2; break;
	//   0xDD is undefined
	case 0xDE: sbc(mem_read8(PC++)); break;
	case 0xDF: push16(PC); PC = 0x18; break;
	case 0xE0: io_write8(mem_read8(PC++), A); break;
	case 0xE1: HL = pop16(); break;
	case 0xE2: mem_write8(0xFF00 + C, A); break;
	case 0xE5: push16(HL); break;
	case 0xE6: F = FLAG_H; A &= mem_read8(PC++); if( A == 0 ) F |= FLAG_Z; break;
	case 0xE7: push16(PC); PC = 0x20; break;
	case 0xE8: // ADD SP, n
		F=0; temp = mem_read8(PC++); 
		if( (SP&0xff) + temp > 0xff )   F |= FLAG_C; 
		if( (SP&0xf)+(temp&0xf) > 0xf ) F |= FLAG_H; 
		SP += (s8)temp;
		break;
	case 0xE9: PC = HL; break;
	case 0xEA: mem_write8(imm16(), A); break;
	//   0xEB, 0xEC, 0xED all undefined
	case 0xEE: F = 0; A ^= mem_read8(PC++); if( A == 0 ) F |= FLAG_Z; break;
	case 0xEF: push16(PC); PC = 0x28; break;
	case 0xF0: A = io_read8(mem_read8(PC++)); break;
	case 0xF1: AF = pop16(); F &= 0xF0; break;
	case 0xF2: A = io_read8(0xFF00 + C); break;
	case 0xF3: break; //TODO: DI
	//   0xF4 undefined
	case 0xF5: push16(AF); break;
	case 0xF6: F = 0; A |= mem_read8(PC++); if( A == 0 ) F |= FLAG_Z; break;
	case 0xF7: push16(PC); PC = 0x30; break;
	case 0xF8: // LD HL, SP+n
		F=0; temp = mem_read8(PC++); 
		if( (SP&0xff) + temp > 0xff )   F |= FLAG_C; 
		if( (SP&0xf)+(temp&0xf) > 0xf ) F |= FLAG_H; 
		HL = SP + (s8)temp;
		break;
	case 0xF9: SP = HL; break;
	case 0xFA: A = mem_read8(imm16()); break;
	case 0xFB: break; //TODO: EI
	//   0xFC and 0xFD undefined		
	case 0xFE: cp(mem_read8(PC++)); break;
	case 0xFF: push16(PC); PC = 0x38; break;
	default: printf("Undefined opcode 0x%X\n", op); exit(1); break;
	}

	return;
}

void cb_prefix(u8 op)
{
	u8 temp = 0;
	
	if( op > 0xBF )
	{ // SET n, r
		int bit = 1 << (op >> 3);
		switch( op & 7 )
		{
		case 0: B |= bit; break;
		case 1: C |= bit; break;
		case 2: D |= bit; break;
		case 3: E |= bit; break;
		case 4: H |= bit; break;
		case 5: L |= bit; break;
		case 6: mem_write8(HL, mem_read8(HL) | bit); break;
		case 7: A |= bit; break;		
		}
		return;	
	}
	
	if( op > 0x7F )
	{ // RES n, r
		int bit = ~(1 << (op >> 3));
		switch( op & 7 )
		{
		case 0: B &= bit; break;
		case 1: C &= bit; break;
		case 2: D &= bit; break;
		case 3: E &= bit; break;
		case 4: H &= bit; break;
		case 5: L &= bit; break;
		case 6: mem_write8(HL, mem_read8(HL) & bit); break;
		case 7: A &= bit; break;		
		}
		return;	
	}
	
	if( op > 0x3F )
	{ // BIT n, r
		F &= ~(FLAG_Z|FLAG_N); F |= FLAG_H;
		int bit = 1 << (op >> 3);
		switch( op & 7 )
		{
		case 0: bit &= B; break;
		case 1: bit &= C; break;
		case 2: bit &= D; break;
		case 3: bit &= E; break;
		case 4: bit &= H; break;
		case 5: bit &= L; break;
		case 6: bit &= mem_read8(HL); break;
		case 7: bit &= A; break;			
		}
		if( bit == 0 ) F |= FLAG_Z;
		return;
	}

	int cf = (F>>4)&1;
	F = 0;
	
	switch( op )
	{
	case 0x00: if( B >> 7 ) F |= FLAG_C; B = (B<<1)|(B>>7); if( B == 0 ) F |= FLAG_Z; break;
	case 0x01: if( C >> 7 ) F |= FLAG_C; C = (C<<1)|(C>>7); if( C == 0 ) F |= FLAG_Z; break;
	case 0x02: if( D >> 7 ) F |= FLAG_C; D = (D<<1)|(D>>7); if( D == 0 ) F |= FLAG_Z; break;
	case 0x03: if( E >> 7 ) F |= FLAG_C; E = (E<<1)|(E>>7); if( E == 0 ) F |= FLAG_Z; break;
	case 0x04: if( H >> 7 ) F |= FLAG_C; H = (H<<1)|(H>>7); if( H == 0 ) F |= FLAG_Z; break;
	case 0x05: if( L >> 7 ) F |= FLAG_C; L = (L<<1)|(L>>7); if( L == 0 ) F |= FLAG_Z; break;
	case 0x06: temp = mem_read8(HL); if( temp >> 7 ) F |= FLAG_C; temp = (temp<<1)|(temp>>7); mem_write8(HL, temp); if( temp == 0 ) F |= FLAG_Z; break;
	case 0x07: if( A >> 7 ) F |= FLAG_C; A = (A<<1)|(A>>7); if( A == 0 ) F |= FLAG_Z; break;
	case 0x08: if( B & 1 ) F |= FLAG_C; B = (B>>1)|(B<<7); if( B == 0 ) F |= FLAG_Z; break;
	case 0x09: if( C & 1 ) F |= FLAG_C; C = (C>>1)|(C<<7); if( C == 0 ) F |= FLAG_Z; break;
	case 0x0A: if( D & 1 ) F |= FLAG_C; D = (D>>1)|(D<<7); if( D == 0 ) F |= FLAG_Z; break;
	case 0x0B: if( E & 1 ) F |= FLAG_C; E = (E>>1)|(E<<7); if( E == 0 ) F |= FLAG_Z; break;
	case 0x0C: if( H & 1 ) F |= FLAG_C; H = (H>>1)|(H<<7); if( H == 0 ) F |= FLAG_Z; break;
	case 0x0D: if( L & 1 ) F |= FLAG_C; L = (L>>1)|(L<<7); if( L == 0 ) F |= FLAG_Z; break;
	case 0x0E: temp = mem_read8(HL); if( temp & 1 ) F |= FLAG_C; temp = (temp>>1)|(temp<<7); mem_write8(HL, temp); if( temp == 0 ) F |= FLAG_Z; break;
	case 0x0F: if( A & 1 ) F |= FLAG_C; A = (A>>1)|(A<<7); if( A == 0 ) F |= FLAG_Z; break;
	case 0x10: if( B >> 7 ) F |= FLAG_C; B = (B<<1)|cf; if( B == 0 ) F |= FLAG_Z; break;
	case 0x11: if( C >> 7 ) F |= FLAG_C; C = (C<<1)|cf; if( C == 0 ) F |= FLAG_Z; break;
	case 0x12: if( D >> 7 ) F |= FLAG_C; D = (D<<1)|cf; if( D == 0 ) F |= FLAG_Z; break;
	case 0x13: if( E >> 7 ) F |= FLAG_C; E = (B<<1)|cf; if( E == 0 ) F |= FLAG_Z; break;
	case 0x14: if( H >> 7 ) F |= FLAG_C; H = (H<<1)|cf; if( H == 0 ) F |= FLAG_Z; break;
	case 0x15: if( L >> 7 ) F |= FLAG_C; L = (L<<1)|cf; if( L == 0 ) F |= FLAG_Z; break;
	case 0x16: temp = mem_read8(HL); if( temp >> 7 ) F |= FLAG_C; temp = (temp<<1)|cf; mem_write8(HL, temp); if( temp == 0 ) F |= FLAG_Z; break;
	case 0x17: if( A >> 7 ) F |= FLAG_C; A = (A<<1)|cf; if( A == 0 ) F |= FLAG_Z; break;
	case 0x18: if( B & 1 ) F |= FLAG_C; B = (B>>1)|(cf<<7); if( B == 0 ) F |= FLAG_Z; break;
	case 0x19: if( C & 1 ) F |= FLAG_C; C = (C>>1)|(cf<<7); if( C == 0 ) F |= FLAG_Z; break;
	case 0x1A: if( D & 1 ) F |= FLAG_C; D = (D>>1)|(cf<<7); if( D == 0 ) F |= FLAG_Z; break;
	case 0x1B: if( E & 1 ) F |= FLAG_C; E = (E>>1)|(cf<<7); if( E == 0 ) F |= FLAG_Z; break;
	case 0x1C: if( H & 1 ) F |= FLAG_C; H = (H>>1)|(cf<<7); if( H == 0 ) F |= FLAG_Z; break;
	case 0x1D: if( L & 1 ) F |= FLAG_C; L = (L>>1)|(cf<<7); if( L == 0 ) F |= FLAG_Z; break;
	case 0x1E: temp = mem_read8(HL); if( temp & 1 ) F |= FLAG_C; temp = (temp>>1)|(cf<<7); mem_write8(HL, temp); if( temp == 0 ) F |= FLAG_Z; break;
	case 0x1F: if( A & 1 ) F |= FLAG_C; A = (A>>1)|(cf<<7); if( A == 0 ) F |= FLAG_Z; break;
	case 0x20: if( B>>7 ) F |= FLAG_C; B <<= 1; if( B == 0 ) F |= FLAG_Z; break;
	case 0x21: if( C>>7 ) F |= FLAG_C; C <<= 1; if( C == 0 ) F |= FLAG_Z; break;
	case 0x22: if( D>>7 ) F |= FLAG_C; D <<= 1; if( D == 0 ) F |= FLAG_Z; break;
	case 0x23: if( E>>7 ) F |= FLAG_C; E <<= 1; if( E == 0 ) F |= FLAG_Z; break;
	case 0x24: if( H>>7 ) F |= FLAG_C; H <<= 1; if( H == 0 ) F |= FLAG_Z; break;
	case 0x25: if( L>>7 ) F |= FLAG_C; L <<= 1; if( L == 0 ) F |= FLAG_Z; break;
	case 0x26: temp = mem_read8(HL); if( temp>>7 ) F |= FLAG_C; temp <<= 1; if( temp == 0 ) F |= FLAG_Z; mem_write8(HL, temp); break;
	case 0x27: if( A>>7 ) F |= FLAG_C; A <<= 1; if( A == 0 ) F |= FLAG_Z; break;
	case 0x28: if( B&1 ) F |= FLAG_C; B = (s8)B>>1; if( B == 0 ) F |= FLAG_Z; break;
	case 0x29: if( C&1 ) F |= FLAG_C; C = (s8)C>>1; if( C == 0 ) F |= FLAG_Z; break;
	case 0x2A: if( D&1 ) F |= FLAG_C; D = (s8)D>>1; if( D == 0 ) F |= FLAG_Z; break;
	case 0x2B: if( E&1 ) F |= FLAG_C; E = (s8)E>>1; if( E == 0 ) F |= FLAG_Z; break;
	case 0x2C: if( H&1 ) F |= FLAG_C; H = (s8)H>>1; if( H == 0 ) F |= FLAG_Z; break;
	case 0x2D: if( L&1 ) F |= FLAG_C; L = (s8)L>>1; if( L == 0 ) F |= FLAG_Z; break;
	case 0x2E: temp = mem_read8(HL); if( temp&1 ) F |= FLAG_C; temp = (s8)temp>>1; if( temp == 0 ) F |= FLAG_Z; mem_write8(HL, temp); break;
	case 0x2F: if( A&1 ) F |= FLAG_C; A = (s8)A>>1; if( A == 0 ) F |= FLAG_Z; break;
	case 0x30: B = (B<<4)|(B>>4); if( B == 0 ) F |= FLAG_Z; break;
	case 0x31: C = (C<<4)|(C>>4); if( C == 0 ) F |= FLAG_Z; break;
	case 0x32: D = (D<<4)|(D>>4); if( D == 0 ) F |= FLAG_Z; break;
	case 0x33: E = (E<<4)|(E>>4); if( E == 0 ) F |= FLAG_Z; break;
	case 0x34: H = (H<<4)|(H>>4); if( H == 0 ) F |= FLAG_Z; break;
	case 0x35: L = (L<<4)|(L>>4); if( L == 0 ) F |= FLAG_Z; break;
	case 0x36: temp = mem_read8(HL); mem_write8(HL, (temp<<4)|(temp>>4)); if( temp == 0 ) F |= FLAG_Z; break;
	case 0x37: A = (A<<4)|(A>>4); if( A == 0 ) F |= FLAG_Z; break;
	case 0x38: if( B&1 ) F |= FLAG_C; B >>= 1; if( B == 0 ) F |= FLAG_Z; break;
	case 0x39: if( C&1 ) F |= FLAG_C; C >>= 1; if( C == 0 ) F |= FLAG_Z; break;
	case 0x3A: if( D&1 ) F |= FLAG_C; D >>= 1; if( D == 0 ) F |= FLAG_Z; break;
	case 0x3B: if( E&1 ) F |= FLAG_C; E >>= 1; if( E == 0 ) F |= FLAG_Z; break;
	case 0x3C: if( H&1 ) F |= FLAG_C; H >>= 1; if( H == 0 ) F |= FLAG_Z; break;
	case 0x3D: if( L&1 ) F |= FLAG_C; L >>= 1; if( L == 0 ) F |= FLAG_Z; break;
	case 0x3E: temp = mem_read8(HL); if( temp&1 ) F |= FLAG_C; temp >>= 1; if( temp == 0 ) F |= FLAG_Z; mem_write8(HL, temp); break;
	case 0x3F: if( A&1 ) F |= FLAG_C; A >>= 1; if( A == 0 ) F |= FLAG_Z; break;
	}

	return;
}

u16 imm16()
{
	u16 retval = mem_read8(PC++);
	retval |= ((u16)mem_read8(PC++)) << 8;
	return retval;
}

void push16(u16 p)
{
	SP--;
	mem_write8(SP, p>>8);
	SP--;
	mem_write8(SP, p&0xff);
	return;
}

u16 pop16()
{
	u16 retval = mem_read8(SP++);
	retval |= ((u16)mem_read8(SP++)) << 8;
	return retval;
}

void adc(u8 p)
{
	int cf = (F & FLAG_C) ? 1 : 0;
	F = 0;
	if( (u16)A + (u16)B + cf > 0xff )  F |= FLAG_C; 
	if( (A&0xf) + (B&0xf) + cf > 0xf ) F |= FLAG_H; 
	A += p + cf;
	if( A == 0 ) F |= FLAG_Z;
	return;
}

void add(u8 p)
{
	F = 0; 
	if( (u16)A + (u16)B > 0xff )  F |= FLAG_C; 
	if( (A&0xf) + (B&0xf) > 0xf ) F |= FLAG_H; 
	A += p; 
	if( A == 0 ) F |= FLAG_Z;
	return;
}

void sub(u8 p)
{
	u16 temp = A - p;
	F = FLAG_N;
	if( (A&0xF) - (p&0xF) > 0xF ) F |= FLAG_H;
	if( temp > 0xFF ) F |= FLAG_C;
	A -= p;
	if( A == 0 ) F |= FLAG_Z;
	return;
}

void sbc(u8 p)
{
	int cf = (F & FLAG_C) ? 1 : 0;
	u16 temp = A - p - cf;
	F = FLAG_N;
	if( (A&0xF) - (p&0xF) - cf > 0xF ) F |= FLAG_H;
	if( temp > 0xFF ) F |= FLAG_C;
	A -= p - cf;
	if( A == 0 ) F |= FLAG_Z;
	return;
}

void cp(u8 p)
{
	u16 temp = A - p;
	F = FLAG_N;
	if( (A&0xF) - (p&0xF) > 0xF ) F |= FLAG_H;
	if( temp > 0xFF ) F |= FLAG_C;
	if( (temp&0xFF) == 0 ) F |= FLAG_Z;
	return;
}

void add_hl(u16 p)
{
	u32 temp = HL;
	temp += p;
	F &= ~(FLAG_C|FLAG_H|FLAG_N);
	if( (HL&0xfff) + (p&0xfff) > 0xfff ) F |= FLAG_H;
	if( temp>>16 ) F |= FLAG_C;
	HL += p;
	return;
}

void daa()
{  // copied from nesdev (https://forums.nesdev.com/viewtopic.php?t=15944)
	if( !(F & FLAG_N) ) 
	{  // after an addition, adjust if (half-)carry occurred or if result is out of bounds
		if((F&FLAG_C) || A > 0x99) 
		{
			A += 0x60; 
			F |= FLAG_C; //c_flag = 1; 
		}
		if((F&FLAG_H) || (A & 0x0f) > 0x09) 
		{
			A += 0x6;
		}
	} else {  // after a subtraction, only adjust if (half-)carry occurred
		if((F&FLAG_C)) { A -= 0x60; }
		if((F&FLAG_H)) { A -= 0x6; }
	}

	// these flags are always updated
	F &= ~(FLAG_Z|FLAG_H);
	if( A == 0 ) F |= FLAG_Z;
	return;
}

extern bool emubios;

void gb_reset()
{
	AF = 0x01B0;
	BC = 0x0013;
	DE = 0x00D8;
	HL = 0x014D;
	SP = 0xFFFE;
	PC = emubios ? 0 : 0x100;
	printf("PC = %x\n", PC);
	return;
}






