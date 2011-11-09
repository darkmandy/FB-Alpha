// FB Alpha Data East Simple 156 System driver module
// Based on MAME driver by 

#include "tiles_generic.h"
#include "arm_intf.h"
#include "msm6295.h"
#include "eeprom.h"
#include "deco16ic.h"

static unsigned char *AllMem;
static unsigned char *MemEnd;
static unsigned char *AllRam;
static unsigned char *RamEnd;
static unsigned char *DrvArmROM;
static unsigned char *DrvGfxROM0;
static unsigned char *DrvGfxROM1;
static unsigned char *DrvGfxROM2;
static unsigned char *DrvSndROM0;
static unsigned char *DrvSndROM1;
static unsigned char *DrvEEPROM;
static unsigned char *DrvArmRAM;
static unsigned char *DrvSysRAM;
static unsigned char *DrvPalRAM;
static unsigned char *DrvSprRAM;

static unsigned int  *DrvPalette;
static unsigned char  DrvRecalc;

static unsigned char DrvJoy1[16];
static unsigned char DrvJoy2[16];
static unsigned char DrvDips[1];
static unsigned short DrvInputs[2];
static unsigned char DrvReset;

static int DrvOkiBank;

static struct BurnInputInfo Simpl156InputList[] = {
	{"P1 Coin",		BIT_DIGITAL,	DrvJoy1 + 0,	"p1 coin"	},
	{"P1 Start",		BIT_DIGITAL,	DrvJoy2 + 7,	"p1 start"	},
	{"P1 Up",		BIT_DIGITAL,	DrvJoy2 + 0,	"p1 up"		},
	{"P1 Down",		BIT_DIGITAL,	DrvJoy2 + 1,	"p1 down"	},
	{"P1 Left",		BIT_DIGITAL,	DrvJoy2 + 2,	"p1 left"	},
	{"P1 Right",		BIT_DIGITAL,	DrvJoy2 + 3,	"p1 right"	},
	{"P1 Button 1",		BIT_DIGITAL,	DrvJoy2 + 4,	"p1 fire 1"	},
	{"P1 Button 2",		BIT_DIGITAL,	DrvJoy2 + 5,	"p1 fire 2"	},
	{"P1 Button 3",		BIT_DIGITAL,	DrvJoy2 + 6,	"p1 fire 3"	},

	{"P2 Coin",		BIT_DIGITAL,	DrvJoy1 + 1,	"p2 coin"	},
	{"P2 Start",		BIT_DIGITAL,	DrvJoy2 + 15,	"p2 start"	},
	{"P2 Up",		BIT_DIGITAL,	DrvJoy2 + 8,	"p2 up"		},
	{"P2 Down",		BIT_DIGITAL,	DrvJoy2 + 9,	"p2 down"	},
	{"P2 Left",		BIT_DIGITAL,	DrvJoy2 + 10,	"p2 left"	},
	{"P2 Right",		BIT_DIGITAL,	DrvJoy2 + 11,	"p2 right"	},
	{"P2 Button 1",		BIT_DIGITAL,	DrvJoy2 + 12,	"p2 fire 1"	},
	{"P2 Button 2",		BIT_DIGITAL,	DrvJoy2 + 13,	"p2 fire 2"	},
	{"P2 Button 3",		BIT_DIGITAL,	DrvJoy2 + 14,	"p2 fire 3"	},

	{"Reset",		BIT_DIGITAL,	&DrvReset,	"reset"		},
	{"Service",		BIT_DIGITAL,	DrvJoy1 + 2,	"service"	},
	{"Dip A",		BIT_DIPSWITCH,	DrvDips + 0,	"dip"		},
};

STDINPUTINFO(Simpl156)

static struct BurnDIPInfo Simpl156DIPList[]=
{
	{0x14, 0xff, 0xff, 0x08, NULL		},

	{0   , 0xfe, 0   ,    2, "Service Mode"	},
	{0x14, 0x01, 0x08, 0x08, "Off"		},
	{0x14, 0x01, 0x08, 0x00, "On"		},
};

STDDIPINFO(Simpl156)

//-----------------------------------------------------------------------------------------------------------------------------------------------

static void oki_set_bank(int bank)
{
	if (DrvOkiBank != (bank & 0x07)) {
		DrvOkiBank = bank & 0x07;
		memcpy (DrvSndROM0 + 0x100000,	DrvSndROM1 + 0x40000 * DrvOkiBank, 0x40000);
	}
}

// use INT type for ADDRESS to kill mingw warnings
static void CommonWrite32(int address, unsigned int data)
{
	Write16Long(DrvArmRAM,			0x000000, 0x007fff) // 16-bit
	Write16Long(DrvSprRAM,			0x010000, 0x011fff) // 16-bit
	Write16Long(DrvPalRAM,			0x020000, 0x020fff) // 16-bit
	Write16Long(((unsigned char*)deco16_pf_control[0]),	0x040000, 0x04001f) // 16-bit
	Write16Long(deco16_pf_ram[0],		0x050000, 0x051fff) // 16-bit
	Write16Long(deco16_pf_ram[0],		0x052000, 0x053fff) // 16-bit mirror
	Write16Long(deco16_pf_ram[1],		0x054000, 0x055fff) // 16-bit
	Write16Long(deco16_pf_rowscroll[0],	0x060000, 0x061fff) // 16-bit
	Write16Long(deco16_pf_rowscroll[1],	0x064000, 0x065fff) // 16-bit

	if (address == 0x030000) {
		oki_set_bank(data);
		EEPROMWrite(data & 0x20, data & 0x40, data & 0x10);
		return;
	}
}

static void CommonWrite8(int address, unsigned char data)
{
	Write16Byte(DrvArmRAM,			0x000000, 0x007fff) // 16-bit
	Write16Byte(DrvSprRAM,			0x010000, 0x011fff) // 16-bit
	Write16Byte(DrvPalRAM,			0x020000, 0x020fff) // 16-bit
	Write16Byte(((unsigned char*)deco16_pf_control[0]),	0x040000, 0x04001f) // 16-bit
	Write16Byte(deco16_pf_ram[0],		0x050000, 0x051fff) // 16-bit
	Write16Byte(deco16_pf_ram[0],		0x052000, 0x053fff) // 16-bit mirror
	Write16Byte(deco16_pf_ram[1],		0x054000, 0x055fff) // 16-bit
	Write16Byte(deco16_pf_rowscroll[0],	0x060000, 0x061fff) // 16-bit
	Write16Byte(deco16_pf_rowscroll[1],	0x064000, 0x065fff) // 16-bit

	if ((address & ~3) == 0x030000) {
		oki_set_bank(data);
		EEPROMWrite(data & 0x20, data & 0x40, data & 0x10);
		return;
	}
}

static unsigned int CommonRead32(int address)
{
	Read16Long(DrvArmRAM,			0x000000, 0x007fff) // 16-bit
	Read16Long(DrvSprRAM,			0x010000, 0x011fff) // 16-bit
	Read16Long(DrvPalRAM,			0x020000, 0x020fff) // 16-bit
	Read16Long(((unsigned char*)deco16_pf_control[0]),	0x040000, 0x04001f) // 16-bit
	Read16Long(deco16_pf_ram[0],		0x050000, 0x051fff) // 16-bit
	Read16Long(deco16_pf_ram[0],		0x052000, 0x053fff) // 16-bit mirror
	Read16Long(deco16_pf_ram[1],		0x054000, 0x055fff) // 16-bit
	Read16Long(deco16_pf_rowscroll[0],	0x060000, 0x061fff) // 16-bit
	Read16Long(deco16_pf_rowscroll[1],	0x064000, 0x065fff) // 16-bit

	if (address == 0x30000) {
		return DrvInputs[1];
	}

	return 0;
}

static unsigned char CommonRead8(int address)
{
	Read16Byte(DrvArmRAM,			0x000000, 0x007fff) // 16-bit
	Read16Byte(DrvSprRAM,			0x010000, 0x011fff) // 16-bit
	Read16Byte(DrvPalRAM,			0x020000, 0x020fff) // 16-bit
	Read16Byte(((unsigned char*)deco16_pf_control[0]),	0x040000, 0x04001f) // 16-bit
	Read16Byte(deco16_pf_ram[0],		0x050000, 0x051fff) // 16-bit
	Read16Byte(deco16_pf_ram[0],		0x052000, 0x053fff) // 16-bit mirror
	Read16Byte(deco16_pf_ram[1],		0x054000, 0x055fff) // 16-bit
	Read16Byte(deco16_pf_rowscroll[0],	0x060000, 0x061fff) // 16-bit
	Read16Byte(deco16_pf_rowscroll[1],	0x064000, 0x065fff) // 16-bit

	switch (address)
	{
		case 0x030000:
		case 0x030001:
		case 0x030002:
		case 0x030003:
			return 0xff;
	}

	return 0;
}

//-----------------------------------------------------------------------------------------------------------------------------------------------

static unsigned int map_offsets[3]; // memory, msm0, msm1

void simpl156_write_byte(unsigned int address, unsigned char data)
{
	if ((address & 0xf80000) == map_offsets[0]) {
		CommonWrite8(address & 0x7ffff, data);
	}

	if ((address & ~3) == map_offsets[1]) {
		MSM6295Command(0, data);
		return;
	}

	if ((address & ~3) == map_offsets[2]) {
		MSM6295Command(1, data);
		return;
	}
}

void simpl156_write_long(unsigned int address, unsigned int data)
{
	if ((address & 0xf80000) == map_offsets[0]) {
		CommonWrite32(address & 0x7ffff, data);
	}

	if (address == map_offsets[1]) {
		MSM6295Command(0, data);
		return;
	}

	if (address == map_offsets[2]) {
		MSM6295Command(1, data);
		return;
	}
}

unsigned char simpl156_read_byte(unsigned int address)
{
	if ((address & 0xf80000) == map_offsets[0]) {
		return CommonRead8(address & 0x7ffff);
	}

	if ((address & ~3) == 0x200000) {
		return ~0;	// inputs -- only read once on boot
	}

	if ((address & ~3) == map_offsets[1]) {
		return MSM6295ReadStatus(0);
	}

	if ((address & ~3) == map_offsets[2]) {
		return MSM6295ReadStatus(1);
	}

	return 0;
}

unsigned int simpl156_read_long(unsigned int address)
{
	if ((address & 0xf80000) == map_offsets[0]) {
		return CommonRead32(address & 0x7ffff);
	}

	if (address == 0x200000) {
		return 0xffff0000 | (EEPROMRead() << 8) | DrvInputs[0] | deco16_vblank; // party time needs proper vblank
	}

	if (address == map_offsets[1]) {
		return MSM6295ReadStatus(0);
	}

	if (address == map_offsets[2]) {
		return MSM6295ReadStatus(1);
	}

	return 0;
}

static void common_map(int ram, int sfx, int mus)
{
	map_offsets[0] = ram; // ram regions
	map_offsets[1] = sfx; // oki #0 (sfx)
	map_offsets[2] = mus; // oki #1 (music)

	ArmInit(1);
	ArmOpen(0);	
	ArmMapMemory(DrvArmROM,		0x000000, 0x07ffff, ARM_ROM);
	ArmMapMemory(DrvSysRAM,		0x201000, 0x201fff, ARM_RAM); // 32-bit
	// ram regions and oki addresses set in map offsets
	ArmSetWriteByteHandler(simpl156_write_byte);
	ArmSetWriteLongHandler(simpl156_write_long);
	ArmSetReadByteHandler(simpl156_read_byte);
	ArmSetReadLongHandler(simpl156_read_long);
	ArmClose();
}

static void mitchell_map() { common_map(0x180000, 0x100000, 0x140000); }
static void magdrop_map()  { common_map(0x380000, 0x400000, 0x340000); }
static void joemacr_map()  { common_map(0x100000, 0x180000, 0x1c0000); }
static void chainrec_map() { common_map(0x400000, 0x480000, 0x3c0000); }
static void magdropp_map() { common_map(0x680000, 0x780000, 0x4c0000); }

static int simpl156_bank_callback(const int bank)
{
	return ((bank >> 4) & 0x7) * 0x1000;
}

static int DrvDoReset()
{
	memset (AllRam, 0, RamEnd - AllRam);

	ArmOpen(0);
	ArmReset();
	ArmClose();

	MSM6295Reset(0);
	MSM6295Reset(1);

	EEPROMReset();

	if (EEPROMAvailable() == 0) {
		EEPROMFill(DrvEEPROM, 0, 0x80);
	}

	deco16Reset();

	DrvOkiBank = -1;
	oki_set_bank(0);

	return 0;
}

static int MemIndex()
{
	unsigned char *Next; Next = AllMem;

	DrvArmROM		= Next; Next += 0x0080000;

	DrvGfxROM0		= Next; Next += 0x0400000;
	DrvGfxROM1		= Next; Next += 0x0400000;
	DrvGfxROM2		= Next; Next += 0x1000000;

	MSM6295ROM		= Next; 
	DrvSndROM0		= Next; Next += 0x0180000;
	DrvSndROM1		= Next; Next += 0x0200000;

	DrvPalette		= (unsigned int*)Next; Next += 0x400 * sizeof(int);

	DrvEEPROM		= Next; Next += 0x0000080;

	AllRam			= Next;

	DrvArmRAM		= Next; Next += 0x0004000;
	DrvSysRAM		= Next; Next += 0x0002000;
	DrvPalRAM		= Next; Next += 0x0001000;
	DrvSprRAM		= Next; Next += 0x0001000;

	RamEnd			= Next;

	MemEnd			= Next;

	return 0;
}

static void decode_samples()
{
	unsigned char *tmp = (unsigned char*)malloc(0x200000);

	for (int i = 0; i < 0x200000; i++) {
		tmp[((i & 1) << 20) | (i >> 1)] = DrvSndROM1[i];
	}

	memcpy (DrvSndROM1, tmp, 0x200000);

	if (tmp) {
		free (tmp);
		tmp = NULL;
	}
}

static void pCommonSpeedhackCallback()
{
	ArmIdleCycles(1120); // ok?
}

static int CommonInit(int (*pRomLoad)(int *, int *), void (*pMap)(), int msm, unsigned int speedhack)
{
	BurnSetRefreshRate(58.00);

	int gfx0len, gfx1len;

	MemIndex();
	int nLen = MemEnd - (unsigned char *)0;
	if ((AllMem = (unsigned char *)malloc(nLen)) == NULL) return 1;
	memset(AllMem, 0, nLen);
	MemIndex();

	if (pRomLoad) {

		if (pRomLoad(&gfx0len, &gfx1len)) return 1;

		deco156_decrypt(DrvArmROM, 0x080000);

		deco56_decrypt_gfx(DrvGfxROM0, gfx0len);

		deco16_tile_decode(DrvGfxROM0, DrvGfxROM1, gfx0len, 0);
		deco16_tile_decode(DrvGfxROM0, DrvGfxROM0, gfx0len, 1);

		deco16_sprite_decode(DrvGfxROM2, gfx1len);

		decode_samples();
	}

	if (pMap) {
		pMap();
	}

	ArmSetSpeedHack(speedhack ? speedhack : ~0, pCommonSpeedhackCallback);

	EEPROMInit(&eeprom_interface_93C46);

	MSM6295Init(0, 1006875 / 132, 60.0, 1);
	MSM6295Init(1, 2013750 / 132 / msm, 20.0, 1);

	deco16Init(1, 0, 1);
	deco16_set_bank_callback(0, simpl156_bank_callback);
	deco16_set_bank_callback(1, simpl156_bank_callback);
	deco16_set_graphics(DrvGfxROM0, gfx0len*2, DrvGfxROM1, gfx0len*2, NULL, 0);
	deco16_set_global_offsets(0, 8);

	GenericTilesInit();

	DrvDoReset();

	return 0;
}

static int DrvExit()
{
	EEPROMExit();

	ArmExit();

	MSM6295Exit(0);
	MSM6295Exit(1);
	MSM6295ROM = NULL;

	GenericTilesExit();

	deco16Exit();

	if (AllMem) {
		free (AllMem);
		AllMem = NULL;
	}

	return 0;
}

static void simpl156_palette_recalc()
{
	unsigned short *p = (unsigned short*)DrvPalRAM;

	for (int i = 0; i < 0x1000 / 4; i++)
	{
		int r = (p[i] >>  0) & 0x1f;
		int g = (p[i] >>  5) & 0x1f;
		int b = (p[i] >> 10) & 0x1f;

		r = (r << 3) | (r >> 2);
		g = (g << 3) | (g >> 2);
		b = (b << 3) | (b >> 2);

		DrvPalette[i] = BurnHighCol(r, g, b, 0);
	}
}

static void draw_sprites()
{
	unsigned short *spriteram = (unsigned short*)DrvSprRAM;

	for (int offs = (0x1400 / 4) - 4; offs >= 0; offs -= 4)
	{
		int mult, inc;

		int sy	  = spriteram[offs + 0];
		if ((sy & 0x1000) && (nCurrentFrame & 1)) continue;

		int code  = spriteram[offs + 1];
		int sx    = spriteram[offs + 2];
		int color = (sx >> 9) & 0x1f;
		int pri   = sx & 0xc000;
		int flipx = sy & 0x2000;
		int flipy = sy & 0x4000;
		int multi = (1 << ((sy & 0x0600) >> 9)) - 1;

		switch (pri)
		{
			case 0x0000: pri = 0x00; break;
			case 0x4000: pri = 0xf0; break;
			case 0x8000:
			case 0xc000: pri = 0xfc; break;
		}

		sx = sx & 0x01ff;
		sy = sy & 0x01ff;
		if (sx >= 320) sx -= 512;
		if (sy >= 256) sy -= 512;
		sy = 240 - sy;
		sx = 304 - sx;

		if (sx > 320) continue;

		code &= ~multi;

		if (flipy) {
			inc = -1;
		} else {
			code += multi;
			inc = 1;
		}

		if (1)//flipscreen
		{
			sy = 240 - sy;
			sx = 304 - sx;
			flipx = !flipx;
			flipy = !flipy;
			mult = 16;
		}
		else
			mult = -16;

		while (multi >= 0)
		{
			deco16_draw_prio_sprite(pTransDraw, DrvGfxROM2, code - multi * inc, (color << 4)+0x200, sx, sy + mult * multi, flipx, flipy, pri);

			multi--;
		}
	}
}

static int DrvDraw()
{
	simpl156_palette_recalc();

	deco16_pf12_update();
	deco16_clear_prio_map();

	for (int i = 0; i < nScreenWidth * nScreenHeight; i++) {
		pTransDraw[i] = 0x100;
	}

	deco16_draw_layer(1, pTransDraw, 2);
	deco16_draw_layer(0, pTransDraw, 4);

	draw_sprites();

	BurnTransferCopy(DrvPalette);

	return 0;
}

static int DrvFrame()
{
	if (DrvReset) {
		DrvDoReset();
	}

	{
		DrvInputs[0] = 0x0007 | (DrvDips[0] & 8);
		DrvInputs[1] = 0xffff;

		for (int i = 0; i < 16; i++) {
			DrvInputs[0] ^= (DrvJoy1[i] & 1) << i;
			DrvInputs[1] ^= (DrvJoy2[i] & 1) << i;
		}
	}

	int nTotalCycles = 28000000 / 58;

	ArmOpen(0);
	deco16_vblank = 0xf0;
	ArmRun(nTotalCycles - 2240);
	ArmSetIRQLine(ARM_IRQ_LINE, ARM_HOLD_LINE);
	deco16_vblank = 0x00;
	ArmRun(2240);
	ArmClose();

	if (pBurnSoundOut) {
		memset (pBurnSoundOut, 0, nBurnSoundLen * sizeof(short) * 2);
		MSM6295Render(0, pBurnSoundOut, nBurnSoundLen);
		MSM6295Render(1, pBurnSoundOut, nBurnSoundLen);
	}

	if (pBurnDraw) {
		DrvDraw();
	}

	return 0;
}

static int DrvScan(int nAction, int *pnMin)
{
	struct BurnArea ba;
	
	if (pnMin != NULL) {
		*pnMin = 0x029707;
	}

	if (nAction & ACB_MEMORY_RAM) {
		memset(&ba, 0, sizeof(ba));
		ba.Data	  = AllRam;
		ba.nLen	  = RamEnd-AllRam;
		ba.szName = "All Ram";
		BurnAcb(&ba);
	}

	if (nAction & ACB_DRIVER_DATA) {
		ArmScan(nAction, pnMin);

		MSM6295Scan(0, nAction);
		MSM6295Scan(1, nAction);

		deco16Scan();

		SCAN_VAR(DrvOkiBank);
	}

	if (nAction & ACB_WRITE) {
		int bank = DrvOkiBank;
		DrvOkiBank = -1;
		oki_set_bank(bank);
	}

	return 0;
}


// Joe & Mac Returns (World, Version 1.1, 1994.05.27)

static struct BurnRomInfo joemacrRomDesc[] = {
	{ "05.u29",			0x080000, 0x74e9a158, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "01.u8l",			0x080000, 0x4da4a2c1, 2 | BRF_GRA },           //  1 Characters & Tiles
	{ "02.u8h",			0x080000, 0x642c08db, 2 | BRF_GRA },           //  2

	{ "mbn01",			0x080000, 0xa3a37353, 3 | BRF_GRA },           //  3 Sprites
	{ "mbn02",			0x080000, 0xaa2230c5, 3 | BRF_GRA },           //  4

	{ "mbn04",			0x040000, 0xdcbd4771, 4 | BRF_SND },           //  5 OKI SFX Samples

	{ "mbn03",			0x200000, 0x70b71a2a, 5 | BRF_SND },           //  6 OKI Music Samples
};

STD_ROM_PICK(joemacr)
STD_ROM_FN(joemacr)

static int joemacrLoadCallback(int *gfxlen0, int *gfxlen1)
{
	if (BurnLoadRom(DrvArmROM  + 0x000000,  0, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM0 + 0x000000,  1, 1)) return 1;
	if (BurnLoadRom(DrvGfxROM0 + 0x080000,  2, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM2 + 0x000001,  3, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x000000,  4, 2)) return 1;

	if (BurnLoadRom(DrvSndROM0 + 0x000000,  5, 1)) return 1;

	if (BurnLoadRom(DrvSndROM1 + 0x000000,  6, 1)) return 1;

	*gfxlen0 = 0x100000;
	*gfxlen1 = 0x200000;

	return 0;
}

static int joemacrInit()
{
	return CommonInit(joemacrLoadCallback, joemacr_map, 1, 0x0284);
}

struct BurnDriver BurnDrvJoemacr = {
	"joemacr", NULL, NULL, NULL, "1994",
	"Joe & Mac Returns (World, Version 1.1, 1994.05.27)\0", NULL, "Data East", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_PLATFORM, 0,
	NULL, joemacrRomInfo, joemacrRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	joemacrInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};


// Joe & Mac Returns (World, Version 1.0, 1994.05.19)

static struct BurnRomInfo joemacraRomDesc[] = {
	{ "mw00",			0x080000, 0xe1b78f40, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mbn00",			0x100000, 0x11b2dac7, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mbn01",			0x080000, 0xa3a37353, 3 | BRF_GRA },           //  2 Sprites
	{ "mbn02",			0x080000, 0xaa2230c5, 3 | BRF_GRA },           //  3

	{ "mbn04",			0x040000, 0xdcbd4771, 4 | BRF_SND },           //  4 OKI SFX Samples

	{ "mbn03",			0x200000, 0x70b71a2a, 5 | BRF_SND },           //  5 OKI Music Samples
};

STD_ROM_PICK(joemacra)
STD_ROM_FN(joemacra)

struct BurnDriver BurnDrvJoemacra = {
	"joemacra", "joemacr", NULL, NULL, "1994",
	"Joe & Mac Returns (World, Version 1.0, 1994.05.19)\0", NULL, "Data East", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_PLATFORM, 0,
	NULL, joemacraRomInfo, joemacraRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	joemacrInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};


// Chain Reaction (World, Version 2.2, 1995.09.25)

static struct BurnRomInfo chainrecRomDesc[] = {
	{ "e1",				0x080000, 0x8a8340ef, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mcc-00",			0x100000, 0x646b03ec, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "u3",				0x080000, 0x92659721, 3 | BRF_GRA },           //  2 Sprites
	{ "u4",				0x080000, 0xe304eb32, 3 | BRF_GRA },           //  3
	{ "u5",				0x080000, 0x1b6f01ea, 3 | BRF_GRA },           //  4
	{ "u6",				0x080000, 0x531a56f2, 3 | BRF_GRA },           //  5

	{ "mcc-04",			0x040000, 0x86ee6ade, 4 | BRF_SND },           //  6 OKI SFX Samples

	{ "mcc-03",			0x100000, 0xda2ebba0, 5 | BRF_SND },           //  7 OKI Music Samples

	{ "eeprom-chainrec.bin",	0x000080, 0xb6da3fbf, 6 | BRF_PRG | BRF_ESS }, //  8 Default EEPROM Data
};

STD_ROM_PICK(chainrec)
STD_ROM_FN(chainrec)

static int chainrecLoadCallback(int *gfxlen0, int *gfxlen1)
{
	if (BurnLoadRom(DrvArmROM  + 0x000000,  0, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM0 + 0x000000,  1, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM2 + 0x000000,  2, 4)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x000002,  3, 4)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x000001,  4, 4)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x000003,  5, 4)) return 1;

	if (BurnLoadRom(DrvSndROM0 + 0x000000,  6, 1)) return 1;

	if (BurnLoadRom(DrvSndROM1 + 0x000000,  7, 1)) return 1;

	if (BurnLoadRom(DrvEEPROM  + 0x000000,  8, 1)) return 1;

	*gfxlen0 = 0x100000;
	*gfxlen1 = 0x200000;

	return 0;
}

static int chainrecInit()
{
	return CommonInit(chainrecLoadCallback, chainrec_map, 1, 0x02d4);
}

struct BurnDriver BurnDrvChainrec = {
	"chainrec", NULL, NULL, NULL, "1995",
	"Chain Reaction (World, Version 2.2, 1995.09.25)\0", NULL, "Data East", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NULL, chainrecRomInfo, chainrecRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	chainrecInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};


// Magical Drop (Japan, Version 1.1, 1995.06.21)

static struct BurnRomInfo magdropRomDesc[] = {
	{ "re00-2.e1",			0x080000, 0x7138f10f, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mcc-00",			0x100000, 0x646b03ec, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mcc-01.a13",			0x100000, 0x13d88745, 3 | BRF_GRA },           //  2 Sprites
	{ "mcc-02.a14",			0x100000, 0xd0f97126, 3 | BRF_GRA },           //  3

	{ "mcc-04",			0x040000, 0x86ee6ade, 4 | BRF_SND },           //  4 OKI SFX Samples

	{ "mcc-03",			0x100000, 0xda2ebba0, 5 | BRF_SND },           //  5 OKI Music Samples

	{ "93c45.2h",			0x000080, 0x16ce8d2d, 6 | BRF_PRG | BRF_ESS }, //  6 Default EEPROM Data
};

STD_ROM_PICK(magdrop)
STD_ROM_FN(magdrop)

static int magdropLoadCallback(int *gfxlen0, int *gfxlen1)
{
	if (BurnLoadRom(DrvArmROM  + 0x000000,  0, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM0 + 0x000000,  1, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM2 + 0x000001,  2, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x000000,  3, 2)) return 1;

	if (BurnLoadRom(DrvSndROM0 + 0x000000,  4, 1)) return 1;

	if (BurnLoadRom(DrvSndROM1 + 0x000000,  5, 1)) return 1;

	if (BurnLoadRom(DrvEEPROM  + 0x000000,  6, 1)) return 1;

	*gfxlen0 = 0x100000;
	*gfxlen1 = 0x200000;

	return 0;
}

static int magdropInit()
{
	return CommonInit(magdropLoadCallback, magdrop_map, 1, 0x02d4);
}

struct BurnDriver BurnDrvMagdrop = {
	"magdrop", "chainrec", NULL, NULL, "1995",
	"Magical Drop (Japan, Version 1.1, 1995.06.21)\0", NULL, "Data East", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NULL, magdropRomInfo, magdropRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	magdropInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};


// Magical Drop Plus 1 (Japan, Version 2.1, 1995.09.12)

static struct BurnRomInfo magdroppRomDesc[] = {
	{ "rz00-1.e1",			0x080000, 0x28caf639, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mcc-00",			0x100000, 0x646b03ec, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mcc-01.a13",			0x100000, 0x13d88745, 3 | BRF_GRA },           //  2 Sprites
	{ "mcc-02.a14",			0x100000, 0xd0f97126, 3 | BRF_GRA },           //  3

	{ "mcc-04",			0x040000, 0x86ee6ade, 4 | BRF_SND },           //  4 OKI SFX Samples

	{ "mcc-03",			0x100000, 0xda2ebba0, 5 | BRF_SND },           //  5 OKI Music Samples

	{ "eeprom.2h",			0x000080, 0xd13d9edd, 6 | BRF_PRG | BRF_ESS }, //  6 Default EEPROM Data
};

STD_ROM_PICK(magdropp)
STD_ROM_FN(magdropp)

static int magdroppInit()
{
	return CommonInit(magdropLoadCallback, magdropp_map, 1, 0x02d4);
}

struct BurnDriver BurnDrvMagdropp = {
	"magdropp", "chainrec", NULL, NULL, "1995",
	"Magical Drop Plus 1 (Japan, Version 2.1, 1995.09.12)\0", NULL, "Data East", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_PUZZLE, 0,
	NULL, magdroppRomInfo, magdroppRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	magdroppInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};


// Charlie Ninja

static struct BurnRomInfo charlienRomDesc[] = {
	{ "nd00-1.1e",			0x080000, 0xf18f4b23, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mbr-00.9a",			0x200000, 0xecf2c7f0, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mbr-01.14a",			0x100000, 0x46c90215, 3 | BRF_GRA },           //  2 Sprites
	{ "mbr-03.14h",			0x100000, 0xc448a68a, 3 | BRF_GRA },           //  3

	{ "nd01-0.13h",			0x040000, 0x635a100a, 4 | BRF_SND },           //  4 OKI SFX Samples

	{ "mbr-02.12f",			0x100000, 0x4f67d333, 5 | BRF_SND },           //  5 OKI Music Samples
};

STD_ROM_PICK(charlien)
STD_ROM_FN(charlien)

static int charlienLoadCallback(int *gfxlen0, int *gfxlen1)
{
	if (BurnLoadRom(DrvArmROM  + 0x000000,  0, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM0 + 0x000000,  1, 1)) return 1;
	for (int i = 0; i < 0x80000; i++) {
		int t = DrvGfxROM0[0x080000 + i];
		DrvGfxROM0[0x080000 + i] = DrvGfxROM0[0x100000 + i];
		DrvGfxROM0[0x100000 + i] = t;
	}

	if (BurnLoadRom(DrvGfxROM2 + 0x000001,  2, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x000000,  3, 2)) return 1;

	if (BurnLoadRom(DrvSndROM0 + 0x000000,  4, 1)) return 1;

	if (BurnLoadRom(DrvSndROM1 + 0x000000,  5, 1)) return 1;

	*gfxlen0 = 0x200000;
	*gfxlen1 = 0x200000;

	return 0;
}

static int charlienInit()
{
	return CommonInit(charlienLoadCallback, mitchell_map, 2, 0xc8c8);
}

struct BurnDriver BurnDrvCharlien = {
	"charlien", NULL, NULL, NULL, "1995",
	"Charlie Ninja\0", NULL, "Mitchell", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_PLATFORM | GBF_SCRFIGHT, 0,
	NULL, charlienRomInfo, charlienRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	charlienInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};


// Party Time: Gonta the Diver II / Ganbare! Gonta!! 2 (World Release)

static struct BurnRomInfo prtytimeRomDesc[] = {
	{ "pz_00-0.1e",			0x080000, 0xec715c87, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mcb-00.9a",			0x200000, 0xc48a4f2b, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mcb-02.14a",			0x200000, 0x423cfb38, 3 | BRF_GRA },           //  2 Sprites
	{ "mcb-05.14h",			0x200000, 0x81540cfb, 3 | BRF_GRA },           //  3
	{ "mcb-01.13a",			0x200000, 0x06f40a57, 3 | BRF_GRA },           //  4
	{ "mcb-03.14d",			0x200000, 0x0aef73af, 3 | BRF_GRA },           //  5

	{ "pz_01-0.13h",		0x040000, 0x8925bce2, 4 | BRF_SND },           //  6 OKI SFX Samples

	{ "mcb-04.12f",			0x200000, 0xe23d3590, 5 | BRF_SND },           //  7 OKI Music Samples

	{ "eeprom-prtytime.bin",	0x000080, 0x105700da, 6 | BRF_PRG | BRF_ESS }, //  8 Default EEPROM Data
};

STD_ROM_PICK(prtytime)
STD_ROM_FN(prtytime)

static int prtytimeLoadCallback(int *gfxlen0, int *gfxlen1)
{
	if (BurnLoadRom(DrvArmROM  + 0x000000,  0, 1)) return 1;

	if (BurnLoadRom(DrvGfxROM0 + 0x000000,  1, 1)) return 1;
	for (int i = 0; i < 0x80000; i++) {
		int t = DrvGfxROM0[0x080000 + i];
		DrvGfxROM0[0x080000 + i] = DrvGfxROM0[0x100000 + i];
		DrvGfxROM0[0x100000 + i] = t;
	}

	if (BurnLoadRom(DrvGfxROM2 + 0x000001,  2, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x000000,  3, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x400001,  4, 2)) return 1;
	if (BurnLoadRom(DrvGfxROM2 + 0x400000,  5, 2)) return 1;

	if (BurnLoadRom(DrvSndROM0 + 0x000000,  6, 1)) return 1;

	if (BurnLoadRom(DrvSndROM1 + 0x000000,  7, 1)) return 1;

	if (BurnLoadRom(DrvEEPROM  + 0x000000,  8, 1)) return 1;

	*gfxlen0 = 0x200000;
	*gfxlen1 = 0x800000;

	return 0;
}

static int prtytimeInit()
{
	return CommonInit(prtytimeLoadCallback, mitchell_map, 2, 0x04f0);
}

struct BurnDriver BurnDrvPrtytime = {
	"prtytime", NULL, NULL, NULL, "1995",
	"Party Time: Gonta the Diver II / Ganbare! Gonta!! 2 (World Release)\0", NULL, "Mitchell", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
	NULL, prtytimeRomInfo, prtytimeRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	prtytimeInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	240, 320, 3, 4
};


// Ganbare! Gonta!! 2 / Party Time: Gonta the Diver II (Japan Release)

static struct BurnRomInfo gangontaRomDesc[] = {
	{ "rd_00-0.1e",			0x080000, 0xf80f43bb, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mcb-00.9a",			0x200000, 0xc48a4f2b, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mcb-02.14a",			0x200000, 0x423cfb38, 3 | BRF_GRA },           //  2 Sprites
	{ "mcb-05.14h",			0x200000, 0x81540cfb, 3 | BRF_GRA },           //  3
	{ "mcb-01.13a",			0x200000, 0x06f40a57, 3 | BRF_GRA },           //  4
	{ "mcb-03.14d",			0x200000, 0x0aef73af, 3 | BRF_GRA },           //  5

	{ "rd_01-0.13h",		0x040000, 0x70fd18c6, 4 | BRF_SND },           //  6 OKI SFX Samples

	{ "mcb-04.12f",			0x200000, 0xe23d3590, 5 | BRF_SND },           //  7 OKI Music Samples

	{ "eeprom-gangonta.bin",	0x000080, 0x27ba60a5, 6 | BRF_PRG | BRF_ESS }, //  8 Default EEPROM Data
};

STD_ROM_PICK(gangonta)
STD_ROM_FN(gangonta)

struct BurnDriver BurnDrvGangonta = {
	"gangonta", "prtytime", NULL, NULL, "1995",
	"Ganbare! Gonta!! 2 / Party Time: Gonta the Diver II (Japan Release)\0", NULL, "Mitchell", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE | BDF_ORIENTATION_VERTICAL | BDF_ORIENTATION_FLIPPED, 2, HARDWARE_MISC_POST90S, GBF_MISC, 0,
	NULL, gangontaRomInfo, gangontaRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	prtytimeInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	240, 320, 3, 4
};


// Osman (World)

static struct BurnRomInfo osmanRomDesc[] = {
	{ "sa00-0.1e",			0x080000, 0xec6b3257, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mcf-00.9a",			0x200000, 0x247712dc, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mcf-02.14a",			0x200000, 0x21251b33, 3 | BRF_GRA },           //  2 Sprites
	{ "mcf-04.14h",			0x200000, 0x4fa55577, 3 | BRF_GRA },           //  3
	{ "mcf-01.13a",			0x200000, 0x83881e25, 3 | BRF_GRA },           //  4
	{ "mcf-03.14d",			0x200000, 0xfaf1d51d, 3 | BRF_GRA },           //  5

	{ "sa01-0.13h",			0x040000, 0xcea8368e, 4 | BRF_SND },           //  6 OKI SFX Samples

	{ "mcf-05.12f",			0x200000, 0xf007d376, 5 | BRF_SND },           //  7 OKI Music Samples

	{ "eeprom-osman.bin",		0x000080, 0x509552b2, 6 | BRF_PRG | BRF_ESS }, //  8 Default EEPROM Data
};

STD_ROM_PICK(osman)
STD_ROM_FN(osman)

static int osmanInit()
{
	return CommonInit(prtytimeLoadCallback, mitchell_map, 2, 0x5974);
}

struct BurnDriver BurnDrvOsman = {
	"osman", NULL, NULL, NULL, "1996",
	"Osman (World)\0", NULL, "Mitchell", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING, 2, HARDWARE_MISC_POST90S, GBF_PLATFORM | GBF_SCRFIGHT, 0,
	NULL, osmanRomInfo, osmanRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	osmanInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};


// Cannon Dancer (Japan)

static struct BurnRomInfo candanceRomDesc[] = {
	{ "sa00-0.1e",			0x080000, 0xec6b3257, 1 | BRF_PRG | BRF_ESS }, //  0 ARM Code

	{ "mcf-00.9a",			0x200000, 0x247712dc, 2 | BRF_GRA },           //  1 Characters & Tiles

	{ "mcf-02.14a",			0x200000, 0x21251b33, 3 | BRF_GRA },           //  2 Sprites
	{ "mcf-04.14h",			0x200000, 0x4fa55577, 3 | BRF_GRA },           //  3
	{ "mcf-01.13a",			0x200000, 0x83881e25, 3 | BRF_GRA },           //  4
	{ "mcf-03.14d",			0x200000, 0xfaf1d51d, 3 | BRF_GRA },           //  5

	{ "sa01-0.13h",			0x040000, 0xcea8368e, 4 | BRF_SND },           //  6 OKI SFX Samples

	{ "mcf-05.12f",			0x200000, 0xf007d376, 5 | BRF_SND },           //  7 OKI Music Samples

	{ "eeprom-candance.bin",	0x000080, 0x0a0a8f6b, 6 | BRF_PRG | BRF_ESS }, //  8 Default EEPROM Data
};

STD_ROM_PICK(candance)
STD_ROM_FN(candance)

struct BurnDriver BurnDrvCandance = {
	"candance", "osman", NULL, NULL, "1996",
	"Cannon Dancer (Japan)\0", NULL, "Mitchell (Atlus license)", "Simple 156",
	NULL, NULL, NULL, NULL,
	BDF_GAME_WORKING | BDF_CLONE, 2, HARDWARE_MISC_POST90S, GBF_PLATFORM | GBF_SCRFIGHT, 0,
	NULL, candanceRomInfo, candanceRomName, NULL, NULL, Simpl156InputInfo, Simpl156DIPInfo,
	osmanInit, DrvExit, DrvFrame, DrvDraw, DrvScan, &DrvRecalc, 0x400,
	320, 240, 4, 3
};
