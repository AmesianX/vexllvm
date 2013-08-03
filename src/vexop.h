#ifndef VEXOP_H
#define VEXOP_H

#include "vexexpr.h"

extern "C" {
#include <valgrind/libvex.h>
#include <valgrind/libvex_ir.h>
}

const char* getVexOpName(IROp op);

class VexExprNaryOp : public VexExpr
{
public:
	VexExprNaryOp(
		VexStmt* in_parent, const IRExpr* expr, unsigned int n_ops);
	VexExprNaryOp(
		VexStmt* in_parent, VexExpr** in_args, unsigned int in_n_ops)
	: 	VexExpr(in_parent),
		op((IROp)~0), n_ops(in_n_ops), args(in_args) {}
	virtual ~VexExprNaryOp(void);
	virtual void print(std::ostream& os) const;
	static VexExpr* createOp(VexStmt* p, const IRExpr* expr);
	virtual const char* getOpName(void) const { return "OP???"; }
protected:
	IROp		op;
	unsigned int	n_ops;
	VexExpr		**args;
};

#define DECL_NARG_OP(x,y)					\
class VexExpr##x##op : public VexExprNaryOp			\
{								\
public:								\
	VexExpr##x##op(VexStmt* p, const IRExpr* expr)		\
	: VexExprNaryOp(p, expr, y) { } \
	VexExpr##x##op(VexStmt* p, VexExpr** args)		\
	: VexExprNaryOp(p, args, y) {}				\
	virtual ~VexExpr##x##op(void) {}			\
}

DECL_NARG_OP(Q, 4);
DECL_NARG_OP(Tri, 3);
DECL_NARG_OP(Bin, 2);
DECL_NARG_OP(Un, 1);

#define OP_CLASS(x,y)				\
class VexExpr##x##y : public VexExpr##x		\
{						\
public:						\
	VexExpr##x##y(VexStmt* in_parent, const IRExpr* expr)	\
	: VexExpr##x(in_parent, expr) { }	\
	VexExpr##x##y(VexStmt* in_parent, VexExpr** args)	\
	: VexExpr##x(in_parent, args) {}			\
	virtual ~VexExpr##x##y() {}		\
	virtual llvm::Value* emit(void) const;	\
	virtual const char* getOpName(void) const { return #y; }	\
private:	\
}

#define UNOP_CLASS(z)	OP_CLASS(Unop, z)
#define BINOP_CLASS(z)	OP_CLASS(Binop, z)
#define TRIOP_CLASS(z)	OP_CLASS(Triop, z)

UNOP_CLASS(32Sto64);
UNOP_CLASS(32Uto64);
UNOP_CLASS(16to8);
UNOP_CLASS(32to8);
UNOP_CLASS(32to16);
UNOP_CLASS(64to32);
UNOP_CLASS(64to8);
UNOP_CLASS(64to1);
UNOP_CLASS(32to1);
UNOP_CLASS(64to16);
UNOP_CLASS(1Uto8);
UNOP_CLASS(1Uto32);
UNOP_CLASS(1Uto64);
UNOP_CLASS(8Uto16);
UNOP_CLASS(8Sto16);
UNOP_CLASS(8Uto32);
UNOP_CLASS(8Sto32);
UNOP_CLASS(8Uto64);
UNOP_CLASS(8Sto64);
UNOP_CLASS(16Uto64);
UNOP_CLASS(16Sto64);
UNOP_CLASS(16Uto32);
UNOP_CLASS(16Sto32);
UNOP_CLASS(32UtoV128);
UNOP_CLASS(64UtoV128);
UNOP_CLASS(V128to64); // lo half
UNOP_CLASS(V128HIto64); // hi half
UNOP_CLASS(128to64); // lo half
UNOP_CLASS(128HIto64); // hi half
BINOP_CLASS(16HLto32);
BINOP_CLASS(32HLto64);
BINOP_CLASS(64HLtoV128);
BINOP_CLASS(64HLto128);
UNOP_CLASS(16HIto8);
UNOP_CLASS(32HIto16);
UNOP_CLASS(64HIto32);
UNOP_CLASS(F32toF64);

UNOP_CLASS(I32StoF64);
UNOP_CLASS(I32UtoF64);


UNOP_CLASS(ReinterpF64asI64);
UNOP_CLASS(ReinterpI64asF64);
UNOP_CLASS(ReinterpF32asI32);
UNOP_CLASS(ReinterpI32asF32);


BINOP_CLASS(F64toF32);
BINOP_CLASS(I64StoF64);
BINOP_CLASS(I64UtoF64);
BINOP_CLASS(F64toI32S);
BINOP_CLASS(F64toI32U);
BINOP_CLASS(F64toI64S);

UNOP_CLASS(Dup8x16);

BINOP_CLASS(GetElem8x8);
BINOP_CLASS(GetElem32x2);

TRIOP_CLASS(SetElem8x8);
TRIOP_CLASS(SetElem32x2);

UNOP_CLASS(Clz64);
UNOP_CLASS(Ctz64);
UNOP_CLASS(Clz32);
UNOP_CLASS(Ctz32);

UNOP_CLASS(Not1);
UNOP_CLASS(Not8);
UNOP_CLASS(Not16);
UNOP_CLASS(Not32);
UNOP_CLASS(Not64);
UNOP_CLASS(NotV128);

BINOP_CLASS(Add8);
BINOP_CLASS(Add16);
BINOP_CLASS(Add32);
BINOP_CLASS(Add64);

BINOP_CLASS(And8);
BINOP_CLASS(And16);
BINOP_CLASS(And32);
BINOP_CLASS(And64);
BINOP_CLASS(AndV128);

BINOP_CLASS(DivModU64to32);
BINOP_CLASS(DivModS64to32);
BINOP_CLASS(DivModU128to64);
BINOP_CLASS(DivModS128to64);
BINOP_CLASS(Min64F0x2);
BINOP_CLASS(Mul64F0x2);
BINOP_CLASS(Div64F0x2);
BINOP_CLASS(Add64F0x2);
BINOP_CLASS(Sub64F0x2);

BINOP_CLASS(Mul32F0x4);
BINOP_CLASS(Div32F0x4);
BINOP_CLASS(Add32F0x4);
BINOP_CLASS(Sub32F0x4);

UNOP_CLASS(Recip32F0x4);
UNOP_CLASS(Sqrt32F0x4);
UNOP_CLASS(RSqrt32F0x4);
UNOP_CLASS(Recip64F0x2);
UNOP_CLASS(Sqrt64F0x2);
UNOP_CLASS(RSqrt64F0x2);


BINOP_CLASS(Max32F0x4);
BINOP_CLASS(Min32F0x4);
BINOP_CLASS(Max64F0x2);

BINOP_CLASS(CmpLT32F0x4);
BINOP_CLASS(CmpLE32F0x4);
BINOP_CLASS(CmpEQ32F0x4);
BINOP_CLASS(CmpUN32F0x4);

BINOP_CLASS(CmpLT64F0x2);
BINOP_CLASS(CmpLE64F0x2);
BINOP_CLASS(CmpEQ64F0x2);
BINOP_CLASS(CmpUN64F0x2);

BINOP_CLASS(SetV128lo64);
BINOP_CLASS(SetV128lo32);
BINOP_CLASS(InterleaveLO8x8);
BINOP_CLASS(InterleaveHI8x8);
BINOP_CLASS(InterleaveLO8x16);
BINOP_CLASS(InterleaveHI8x16);
BINOP_CLASS(InterleaveLO16x4);
BINOP_CLASS(InterleaveHI16x4);
BINOP_CLASS(InterleaveLO16x8);
BINOP_CLASS(InterleaveHI16x8);
BINOP_CLASS(InterleaveLO64x2);
BINOP_CLASS(InterleaveHI64x2);
BINOP_CLASS(InterleaveLO32x4);
BINOP_CLASS(InterleaveHI32x4);
BINOP_CLASS(InterleaveLO32x2);
BINOP_CLASS(InterleaveHI32x2);

BINOP_CLASS(Mul8);
BINOP_CLASS(Mul16);
BINOP_CLASS(Mul32);
BINOP_CLASS(Mul64);

BINOP_CLASS(MullS8);
BINOP_CLASS(MullS16);
BINOP_CLASS(MullS32);
BINOP_CLASS(MullS64);

BINOP_CLASS(MullU8);
BINOP_CLASS(MullU16);
BINOP_CLASS(MullU32);
BINOP_CLASS(MullU64);

BINOP_CLASS(Max8Ux8);
BINOP_CLASS(Max8Ux16);
BINOP_CLASS(Max16Sx4);
BINOP_CLASS(Max16Sx8);

BINOP_CLASS(Min8Ux8);
BINOP_CLASS(Min8Ux16);
BINOP_CLASS(Min16Sx4);
BINOP_CLASS(Min16Sx8);


UNOP_CLASS(Sqrt32Fx4);
UNOP_CLASS(Sqrt64Fx2);

UNOP_CLASS(RSqrt32Fx4);
UNOP_CLASS(RSqrt64Fx2);

UNOP_CLASS(Recip32Fx4);
UNOP_CLASS(Recip64Fx2);


BINOP_CLASS(Min32Fx2);
BINOP_CLASS(Max32Fx2);
BINOP_CLASS(Min32Fx4);
BINOP_CLASS(Max32Fx4);

BINOP_CLASS(Max64Fx2);
BINOP_CLASS(Min64Fx2);

BINOP_CLASS(Or8);
BINOP_CLASS(Or16);
BINOP_CLASS(Or32);
BINOP_CLASS(Or64);
BINOP_CLASS(OrV128);

BINOP_CLASS(Shl8);
BINOP_CLASS(Shl16);
BINOP_CLASS(Shl32);
BINOP_CLASS(Shl64);

BINOP_CLASS(Sar8);
BINOP_CLASS(Sar16);
BINOP_CLASS(Sar32);
BINOP_CLASS(Sar64);

BINOP_CLASS(Shr8);
BINOP_CLASS(Shr16);
BINOP_CLASS(Shr32);
BINOP_CLASS(Shr64);

BINOP_CLASS(Sub8);
BINOP_CLASS(Sub16);
BINOP_CLASS(Sub32);
BINOP_CLASS(Sub64);

BINOP_CLASS(Xor8);
BINOP_CLASS(Xor16);
BINOP_CLASS(Xor32);
BINOP_CLASS(Xor64);
BINOP_CLASS(XorV128);

BINOP_CLASS(CmpEQ8);
BINOP_CLASS(CmpEQ16);
BINOP_CLASS(CmpEQ32);
BINOP_CLASS(CmpEQ64);
BINOP_CLASS(CmpNE8);
BINOP_CLASS(CmpNE16);
BINOP_CLASS(CmpNE32);
BINOP_CLASS(CmpNE64);

BINOP_CLASS(CasCmpEQ8);
BINOP_CLASS(CasCmpEQ16);
BINOP_CLASS(CasCmpEQ32);
BINOP_CLASS(CasCmpEQ64);
BINOP_CLASS(CasCmpNE8);
BINOP_CLASS(CasCmpNE16);
BINOP_CLASS(CasCmpNE32);
BINOP_CLASS(CasCmpNE64);


BINOP_CLASS(CmpF64);

BINOP_CLASS(CmpLE64S);
BINOP_CLASS(CmpLE64U);
BINOP_CLASS(CmpLT64S);
BINOP_CLASS(CmpLT64U);
BINOP_CLASS(CmpLE32S);
BINOP_CLASS(CmpLE32U);
BINOP_CLASS(CmpLT32S);
BINOP_CLASS(CmpLT32U);

BINOP_CLASS(Shl8x8);
BINOP_CLASS(Shr8x8);
BINOP_CLASS(Sar8x8);
BINOP_CLASS(Sal8x8);
BINOP_CLASS(SarN8x8);
BINOP_CLASS(ShlN8x8);
BINOP_CLASS(ShrN8x8);

BINOP_CLASS(Shl8x16);
BINOP_CLASS(Shr8x16);
BINOP_CLASS(Sar8x16);
BINOP_CLASS(Sal8x16);
BINOP_CLASS(SarN8x16);
BINOP_CLASS(ShlN8x16);
BINOP_CLASS(ShrN8x16);

BINOP_CLASS(Shl16x4);
BINOP_CLASS(Shr16x4);
BINOP_CLASS(Sar16x4);
BINOP_CLASS(Sal16x4);
BINOP_CLASS(ShlN16x4);
BINOP_CLASS(ShrN16x4);
BINOP_CLASS(SarN16x4);

BINOP_CLASS(Shl16x8);
BINOP_CLASS(Shr16x8);
BINOP_CLASS(Sar16x8);
BINOP_CLASS(Sal16x8);
BINOP_CLASS(ShlN16x8);
BINOP_CLASS(ShrN16x8);
BINOP_CLASS(SarN16x8);

BINOP_CLASS(Shl32x2);
BINOP_CLASS(Shr32x2);
BINOP_CLASS(Sar32x2);
BINOP_CLASS(Sal32x2);
BINOP_CLASS(ShlN32x2);
BINOP_CLASS(ShrN32x2);
BINOP_CLASS(SarN32x2);

BINOP_CLASS(Shl32x4);
BINOP_CLASS(Shr32x4);
BINOP_CLASS(Sar32x4);
BINOP_CLASS(Sal32x4);
BINOP_CLASS(ShlN32x4);
BINOP_CLASS(ShrN32x4);
BINOP_CLASS(SarN32x4);

BINOP_CLASS(Shl64x2);
BINOP_CLASS(Shr64x2);
BINOP_CLASS(Sar64x2);
BINOP_CLASS(Sal64x2);
BINOP_CLASS(ShlN64x2);
BINOP_CLASS(ShrN64x2);
BINOP_CLASS(SarN64x2);

BINOP_CLASS(Add8x8);
BINOP_CLASS(Add8x16);
BINOP_CLASS(Add16x4);
BINOP_CLASS(Add16x8);
BINOP_CLASS(Add32x2);
BINOP_CLASS(Add32x4);
BINOP_CLASS(Add64x2);

BINOP_CLASS(Sub8x8);
BINOP_CLASS(Sub8x16);
BINOP_CLASS(Sub16x4);
BINOP_CLASS(Sub16x8);
BINOP_CLASS(Sub32x2);
BINOP_CLASS(Sub32x4);
BINOP_CLASS(Sub64x2);

BINOP_CLASS(Mul8x8);
BINOP_CLASS(Mul8x16);
BINOP_CLASS(Mul16x4);
BINOP_CLASS(Mul16x8);
BINOP_CLASS(Mul32x2);
BINOP_CLASS(Mul32x4);

BINOP_CLASS(MulHi16Ux4);
BINOP_CLASS(MulHi16Sx4);
BINOP_CLASS(MulHi16Ux8);
BINOP_CLASS(MulHi16Sx8);
BINOP_CLASS(MulHi32Ux4);
BINOP_CLASS(MulHi32Sx4);

#if 0
#define Iop_QNarrow16Sx4	Iop_QNarrowBin16Sto8Sx8
#define Iop_QNarrow16Sx8	Iop_QNarrowBin16Sto8Sx16
#define Iop_QNarrow32Sx2	Iop_QNarrowBin32Sto16Sx4
#define Iop_QNarrow32Sx4	Iop_QNarrowBin32Sto16Sx8
#define Iop_QNarrow16Ux4	Iop_QNarrowUn16Sto8Ux8
#define Iop_QNarrow16Ux8	Iop_QNarrowBin16Sto8Ux16
#define Iop_QNarrow32Ux4	Iop_QNarrowBin32Sto16Ux8
#define Iop_Narrow16x8		Iop_NarrowBin16to8x16
#define Iop_Narrow32x4		Iop_NarrowBin32to16x8
#endif



BINOP_CLASS(NarrowBin16to8x16);
BINOP_CLASS(NarrowBin32to16x8);

BINOP_CLASS(QNarrowBin16Sto8Sx8);
BINOP_CLASS(QNarrowBin16Sto8Sx16);
BINOP_CLASS(QNarrowBin32Sto16Sx4);
BINOP_CLASS(QNarrowBin32Sto16Sx8);
BINOP_CLASS(QNarrowUn16Sto8Ux8);
BINOP_CLASS(QNarrowBin16Sto8Ux16);
BINOP_CLASS(QNarrowBin32Sto16Ux8);

BINOP_CLASS(QAdd64Sx1);
BINOP_CLASS(QAdd64Sx2);
BINOP_CLASS(QAdd64Ux1);
BINOP_CLASS(QAdd64Ux2);

BINOP_CLASS(QAdd32Sx2);
BINOP_CLASS(QAdd32Sx4);
BINOP_CLASS(QAdd32Ux2);
BINOP_CLASS(QAdd32Ux4);

BINOP_CLASS(QAdd16Sx2);
BINOP_CLASS(QAdd16Sx4);
BINOP_CLASS(QAdd16Sx8);
BINOP_CLASS(QAdd16Ux2);
BINOP_CLASS(QAdd16Ux4);
BINOP_CLASS(QAdd16Ux8);

BINOP_CLASS(QAdd8Sx4);
BINOP_CLASS(QAdd8Sx8);
BINOP_CLASS(QAdd8Sx16);
BINOP_CLASS(QAdd8Ux4);
BINOP_CLASS(QAdd8Ux8);
BINOP_CLASS(QAdd8Ux16);

BINOP_CLASS(QSub64Sx1);
BINOP_CLASS(QSub64Sx2);
BINOP_CLASS(QSub64Ux1);
BINOP_CLASS(QSub64Ux2);

BINOP_CLASS(QSub32Sx2);
BINOP_CLASS(QSub32Sx4);
BINOP_CLASS(QSub32Ux2);
BINOP_CLASS(QSub32Ux4);

BINOP_CLASS(QSub16Sx2);
BINOP_CLASS(QSub16Sx4);
BINOP_CLASS(QSub16Sx8);
BINOP_CLASS(QSub16Ux2);
BINOP_CLASS(QSub16Ux4);
BINOP_CLASS(QSub16Ux8);

BINOP_CLASS(QSub8Sx4);
BINOP_CLASS(QSub8Sx8);
BINOP_CLASS(QSub8Sx16);
BINOP_CLASS(QSub8Ux4);
BINOP_CLASS(QSub8Ux8);
BINOP_CLASS(QSub8Ux16);

BINOP_CLASS(Avg8Ux8); 
BINOP_CLASS(Avg8Ux16);
BINOP_CLASS(Avg8Sx16);
BINOP_CLASS(Avg16Ux4);
BINOP_CLASS(Avg16Ux8);
BINOP_CLASS(Avg16Sx8);
BINOP_CLASS(Avg32Ux4);
BINOP_CLASS(Avg32Sx4);

/* 8x4; 16x2 signed/unsigned halving add/sub. 
 * For each lane in 8x4,
 * these compute bits 8:1 of (eg) sx(argL) + sx(argR)
 * or zx(argL) - zx(argR) etc. */
BINOP_CLASS(HAdd16Ux2);
BINOP_CLASS(HAdd16Sx2);
BINOP_CLASS(HSub16Ux2);
BINOP_CLASS(HSub16Sx2);
BINOP_CLASS(HAdd8Ux4);
BINOP_CLASS(HAdd8Sx4);
BINOP_CLASS(HSub8Ux4);
BINOP_CLASS(HSub8Sx4);

BINOP_CLASS(CmpEQ8x8);
BINOP_CLASS(CmpEQ8x16);
BINOP_CLASS(CmpEQ16x4);
BINOP_CLASS(CmpEQ16x8);
BINOP_CLASS(CmpEQ32x2);
BINOP_CLASS(CmpEQ32x4);

BINOP_CLASS(CmpGT8Sx8);
BINOP_CLASS(CmpGT8Sx16);
BINOP_CLASS(CmpGT16Sx4);
BINOP_CLASS(CmpGT16Sx8);
BINOP_CLASS(CmpGT32Sx2);
BINOP_CLASS(CmpGT32Sx4);
BINOP_CLASS(CmpGT64Sx2);

BINOP_CLASS(CmpGT8Ux8);
BINOP_CLASS(CmpGT8Ux16);
BINOP_CLASS(CmpGT16Ux4);
BINOP_CLASS(CmpGT16Ux8);
BINOP_CLASS(CmpGT32Ux2);
BINOP_CLASS(CmpGT32Ux4);

BINOP_CLASS(CmpEQ32Fx2);
BINOP_CLASS(CmpEQ32Fx4);
BINOP_CLASS(CmpEQ64Fx2);

BINOP_CLASS(CmpGT32Fx2);
BINOP_CLASS(CmpGT32Fx4);
BINOP_CLASS(CmpGT64Fx2);

BINOP_CLASS(CmpGE32Fx2);
BINOP_CLASS(CmpGE32Fx4);
BINOP_CLASS(CmpGE64Fx2);

BINOP_CLASS(CmpLT32Fx2);
BINOP_CLASS(CmpLT32Fx4);
BINOP_CLASS(CmpLT64Fx2);

BINOP_CLASS(CmpLE32Fx2);
BINOP_CLASS(CmpLE32Fx4);
BINOP_CLASS(CmpLE64Fx2);

BINOP_CLASS(CmpUN32Fx2);
BINOP_CLASS(CmpUN32Fx4);
BINOP_CLASS(CmpUN64Fx2);


BINOP_CLASS(Add32Fx2);
BINOP_CLASS(Add32Fx4);
BINOP_CLASS(Add64Fx2);

BINOP_CLASS(Sub32Fx2);
BINOP_CLASS(Sub32Fx4);
BINOP_CLASS(Sub64Fx2);

BINOP_CLASS(Mul32Fx2);
BINOP_CLASS(Mul32Fx4);
BINOP_CLASS(Mul64Fx2);

BINOP_CLASS(Div32Fx4);
BINOP_CLASS(Div64Fx2);

UNOP_CLASS(Neg32Fx2);
UNOP_CLASS(Neg32Fx4);

TRIOP_CLASS(AddF32);
TRIOP_CLASS(SubF32);
TRIOP_CLASS(DivF32);
TRIOP_CLASS(MulF32);
UNOP_CLASS(NegF32);

TRIOP_CLASS(AddF64);
TRIOP_CLASS(SubF64);
TRIOP_CLASS(DivF64);
TRIOP_CLASS(MulF64);
UNOP_CLASS(NegF64);

BINOP_CLASS(RoundF64toInt);
BINOP_CLASS(RoundF32toInt);

BINOP_CLASS(Perm8x8);
BINOP_CLASS(Perm8x16);

TRIOP_CLASS(PRemF64);
TRIOP_CLASS(PRemC3210F64);

#endif //VEXOP_H
