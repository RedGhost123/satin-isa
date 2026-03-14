/*
 * isa_packet_v2.h
 * SATIN Core Gen 3 — ISA v2.0
 *
 * 256-bit instruction word. BACKWARD COMPATIBLE with ISA v0.1.
 *
 * All ISA v0.1 fields occupy the SAME bit positions (bits [255:128]).
 * v0.1 programs compiled with isa_packet.h run on Gen 3 hardware unchanged:
 *   - bytes [0..15] (bits [255:128]) are identical in layout to v0.1
 *   - bytes [16..31] (bits [127:0])  are NEW — always zero in v0.1 programs
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * ENDIANNESS
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * Big-endian in memory. bit [255] = MSB of byte[0]. bit [0] = LSB of byte[31].
 * All multi-byte fields stored big-endian: MSB at lower byte offset.
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * INSTRUCTION WORD LAYOUT — bytes [0..15]  (ISA v0.1 COMPATIBLE REGION)
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *  Bits        Bytes    Width  Field           Description
 *  ──────────  ───────  ─────  ──────────────  ─────────────────────────────
 *  [255:248]   [ 0]      8     opcode          Instruction opcode
 *  [247:240]   [ 1]      8     dst_tile_id     Destination tile (0–255)
 *  [239:232]   [ 2]      8     src_a_id        Source tile A  (0–255)
 *  [231:224]   [ 3]      8     src_b_id        Source tile B  (0–255)
 *  [223:216]   [ 4]      8     src_c_id        Source tile C  (0–255)
 *  [215:200]   [ 5: 6]  16     scalar_imm      FP16 scalar immediate
 *  [199:168]   [ 7:10]  32     host_addr       Host memory byte address  ← WIDENED 16→32
 *  [167:136]   [11:14]  32     sram_addr       SRAM/DRAM byte address    ← WIDENED 16→32
 *  [135:128]   [15]      8     byte_count_hi   byte_count bits [31:24]   ← SPLIT: hi byte
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * INSTRUCTION WORD LAYOUT — bytes [16..31]  (ISA v2.0 NEW REGION)
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *  Bits        Bytes    Width  Field           Description
 *  ──────────  ───────  ─────  ──────────────  ─────────────────────────────
 *  [127:104]   [16:18]  24     byte_count_lo   byte_count bits [23:0]  (combined with hi → 32-bit)
 *  [103: 96]   [19]      8     aux_field       Axis / dtype / causal / precision mode
 *  [ 95: 80]   [20:21]  16     tile_M          Runtime systolic M dimension  (NEW)
 *  [ 79: 64]   [22:23]  16     tile_N          Runtime systolic N dimension  (NEW)
 *  [ 63: 48]   [24:25]  16     tile_K          Runtime systolic K dimension  (NEW)
 *  [ 47: 32]   [26:27]  16     head_count      Attention heads H             (NEW)
 *  [ 31: 16]   [28:29]  16     core_mask       Bitmask: which of 8 cores run (NEW)
 *  [ 15:  0]   [30:31]  16     reserved        Must be zero
 *
 * NOTE: byte_count is now 32-bit, split across bytes [15] (hi) and [16..18] (lo).
 * Use ISA2_GET_BYTE_COUNT() / ISA2_SET_BYTE_COUNT() macros — do not access raw bytes.
 *
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * VISUAL BIT MAP (v2.0, 32 bytes)
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *
 *  [0]      [1]      [2]      [3]      [4]      [5]      [6]
 *  OPCODE   DST_ID   SRC_A    SRC_B    SRC_C    ──SCALAR_IMM──
 *
 *  [7]      [8]      [9]      [10]     [11]     [12]     [13]     [14]
 *  ──────────────HOST_ADDR[31:0]──────  ───────────SRAM_ADDR[31:0]──────
 *
 *  [15]     [16]     [17]     [18]     [19]     [20]     [21]
 *  BC[31:24] ──────BC[23:0]──────────  AUX      ──TILE_M[15:0]──
 *
 *  [22]     [23]     [24]     [25]     [26]     [27]     [28]     [29]     [30:31]
 *  ──TILE_N─  ──TILE_K──      ──HEAD_COUNT──   ──CORE_MASK──     RESERVED
 *
 */

#ifndef SATIN_ISA_PACKET_V2_H
#define SATIN_ISA_PACKET_V2_H

#include <stdint.h>
#include <string.h>

/* ─────────────────────────────────────────────────────────────────────────── */
/* Instruction word constants                                                  */
/* ─────────────────────────────────────────────────────────────────────────── */

#define SATIN_INSTR_BYTES_V2    32u     /* instruction word size in bytes (v2.0) */
#define SATIN_INSTR_BITS_V2     256u    /* instruction word size in bits  (v2.0) */
#define SATIN_ISA_VERSION       0x0200u /* v2.0 = 0x0200                         */

/* v0.1 size kept for backward compat */
#define SATIN_INSTR_BYTES_V1    16u
#define SATIN_INSTR_BITS_V1     128u

/* ─────────────────────────────────────────────────────────────────────────── */
/* Byte offsets — upper word (bytes 0–15, v0.1 compatible region)              */
/* ─────────────────────────────────────────────────────────────────────────── */

#define IOFF2_OPCODE            0u   /* byte[0]     opcode [7:0]               */
#define IOFF2_DST_TILE_ID       1u   /* byte[1]     dst_tile_id [7:0]          */
#define IOFF2_SRC_A_ID          2u   /* byte[2]     src_a_id [7:0]             */
#define IOFF2_SRC_B_ID          3u   /* byte[3]     src_b_id [7:0]             */
#define IOFF2_SRC_C_ID          4u   /* byte[4]     src_c_id [7:0]             */
#define IOFF2_SCALAR_IMM_HI     5u   /* byte[5]     scalar_imm [15:8]          */
#define IOFF2_SCALAR_IMM_LO     6u   /* byte[6]     scalar_imm [7:0]           */
#define IOFF2_HOST_ADDR_B3      7u   /* byte[7]     host_addr [31:24]          */
#define IOFF2_HOST_ADDR_B2      8u   /* byte[8]     host_addr [23:16]          */
#define IOFF2_HOST_ADDR_B1      9u   /* byte[9]     host_addr [15:8]           */
#define IOFF2_HOST_ADDR_B0      10u  /* byte[10]    host_addr [7:0]            */
#define IOFF2_SRAM_ADDR_B3      11u  /* byte[11]    sram_addr [31:24]          */
#define IOFF2_SRAM_ADDR_B2      12u  /* byte[12]    sram_addr [23:16]          */
#define IOFF2_SRAM_ADDR_B1      13u  /* byte[13]    sram_addr [15:8]           */
#define IOFF2_SRAM_ADDR_B0      14u  /* byte[14]    sram_addr [7:0]            */
#define IOFF2_BYTE_COUNT_B3     15u  /* byte[15]    byte_count [31:24]  (hi)   */

/* ─────────────────────────────────────────────────────────────────────────── */
/* Byte offsets — lower word (bytes 16–31, v2.0 new region)                    */
/* ─────────────────────────────────────────────────────────────────────────── */

#define IOFF2_BYTE_COUNT_B2     16u  /* byte[16]    byte_count [23:16]         */
#define IOFF2_BYTE_COUNT_B1     17u  /* byte[17]    byte_count [15:8]          */
#define IOFF2_BYTE_COUNT_B0     18u  /* byte[18]    byte_count [7:0]   (lo)    */
#define IOFF2_AUX_FIELD         19u  /* byte[19]    aux_field [7:0]            */
#define IOFF2_TILE_M_HI         20u  /* byte[20]    tile_M [15:8]              */
#define IOFF2_TILE_M_LO         21u  /* byte[21]    tile_M [7:0]               */
#define IOFF2_TILE_N_HI         22u  /* byte[22]    tile_N [15:8]              */
#define IOFF2_TILE_N_LO         23u  /* byte[23]    tile_N [7:0]               */
#define IOFF2_TILE_K_HI         24u  /* byte[24]    tile_K [15:8]              */
#define IOFF2_TILE_K_LO         25u  /* byte[25]    tile_K [7:0]               */
#define IOFF2_HEAD_COUNT_HI     26u  /* byte[26]    head_count [15:8]          */
#define IOFF2_HEAD_COUNT_LO     27u  /* byte[27]    head_count [7:0]           */
#define IOFF2_CORE_MASK_HI      28u  /* byte[28]    core_mask [15:8]           */
#define IOFF2_CORE_MASK_LO      29u  /* byte[29]    core_mask [7:0]            */
#define IOFF2_RESERVED_HI       30u  /* byte[30]    reserved [15:8] — zero     */
#define IOFF2_RESERVED_LO       31u  /* byte[31]    reserved [7:0]  — zero     */

/* ─────────────────────────────────────────────────────────────────────────── */
/* Field widths (bits)                                                          */
/* ─────────────────────────────────────────────────────────────────────────── */

#define IWIDTH2_OPCODE          8u
#define IWIDTH2_DST_TILE_ID     8u
#define IWIDTH2_SRC_A_ID        8u
#define IWIDTH2_SRC_B_ID        8u
#define IWIDTH2_SRC_C_ID        8u
#define IWIDTH2_SCALAR_IMM      16u
#define IWIDTH2_HOST_ADDR       32u  /* was 16 in v0.1 */
#define IWIDTH2_SRAM_ADDR       32u  /* was 16 in v0.1 */
#define IWIDTH2_BYTE_COUNT      32u  /* was 16 in v0.1 */
#define IWIDTH2_AUX_FIELD       8u
#define IWIDTH2_TILE_M          16u
#define IWIDTH2_TILE_N          16u
#define IWIDTH2_TILE_K          16u
#define IWIDTH2_HEAD_COUNT      16u
#define IWIDTH2_CORE_MASK       16u
#define IWIDTH2_RESERVED        16u

/* ─────────────────────────────────────────────────────────────────────────── */
/* Opcode table — ISA v0.1 opcodes (UNCHANGED, same hex values)                */
/* ─────────────────────────────────────────────────────────────────────────── */

/* ── DMA / Memory ─────────────────────────────────────────────────────────── */
#define OPC2_NOP                0x00u  /* No operation                          */
#define OPC2_DMA_LOAD           0x01u  /* Host memory → SRAM/DRAM               */
#define OPC2_DMA_STORE          0x02u  /* SRAM/DRAM → Host memory               */
#define OPC2_TILE_LOAD          0x03u  /* SRAM → Tile register                  */
#define OPC2_TILE_STORE         0x04u  /* Tile register → SRAM                  */

/* ── Compute (v0.1) ───────────────────────────────────────────────────────── */
#define OPC2_MATMUL_I8          0x10u  /* INT8 matrix multiply, saturating      */
#define OPC2_MATMUL_F16         0x11u  /* FP16 matrix multiply                  */
#define OPC2_TRANSPOSE          0x12u  /* 2-D tile transpose                    */
#define OPC2_SCALE              0x13u  /* Per-element FP16 scalar multiply      */
#define OPC2_RELU               0x14u  /* ReLU activation                       */
#define OPC2_GELU               0x15u  /* GELU activation                       */
#define OPC2_SOFTMAX            0x16u  /* Softmax along axis in aux[0]          */
#define OPC2_LAYERNORM          0x17u  /* Layer normalisation                   */
#define OPC2_ELEM_ADD           0x18u  /* Element-wise addition                 */
#define OPC2_ELEM_MUL           0x19u  /* Element-wise multiplication           */
#define OPC2_CAST               0x1Au  /* Type cast; target dtype in aux[1:0]   */
#define OPC2_QUANTIZE           0x1Bu  /* FP16 → INT8                           */
#define OPC2_DEQUANTIZE         0x1Cu  /* INT8 → FP16                           */
#define OPC2_ATTENTION          0x20u  /* Fused scaled dot-product attention    */
#define OPC2_TILE_ZERO          0xFEu  /* Zero a tile register                  */
#define OPC2_HALT               0xFFu  /* Halt; halt_status in aux_field        */

/* ── NEW opcodes — ISA v2.0 ───────────────────────────────────────────────── */
#define OPC2_FLASH_ATTN         0x30u  /* Flash Attention (no S×S materialise)  */
#define OPC2_KV_APPEND          0x31u  /* Append K,V to KV cache at cur pos     */
#define OPC2_EMBED_LOOKUP       0x32u  /* Token embedding fetch from DRAM       */
#define OPC2_GATHER             0x33u  /* Index-based gather (MoE routing)      */
#define OPC2_SCATTER            0x34u  /* Index-based scatter                   */
#define OPC2_ROPE_ENCODE        0x35u  /* Rotary Position Embedding in-place    */
#define OPC2_BARRIER            0x36u  /* Memory fence: wait all DRAM ops done  */
#define OPC2_LOOP_BEGIN         0x37u  /* Start counted loop (count in tile_M)  */
#define OPC2_LOOP_END           0x38u  /* Decrement counter; jump if non-zero   */
#define OPC2_BF16_CAST          0x39u  /* Cast to/from BFloat16                 */
#define OPC2_SPARSE_MATMUL      0x3Au  /* 2:4 sparse matmul (2× throughput)     */
#define OPC2_FP8_CAST           0x3Bu  /* Cast to/from FP8 E4M3 or E5M2        */
#define OPC2_INT4_PACK          0x3Cu  /* Pack two INT4 values into INT8 slot   */
#define OPC2_INT4_UNPACK        0x3Du  /* Unpack INT4 from INT8 storage         */
#define OPC2_CERT_HASH          0x3Eu  /* SHA-256 of tile → cert register       */
#define OPC2_MULTI_HEAD_ATTN    0x3Fu  /* Multi-head attention (any S,D,H)      */
#define OPC2_PREFETCH           0x40u  /* Hint: begin DRAM→SRAM prefetch        */
#define OPC2_MATMUL_BF16        0x41u  /* BF16 matrix multiply                  */
#define OPC2_MATMUL_FP8         0x42u  /* FP8 matrix multiply                   */
#define OPC2_MATMUL_I4          0x43u  /* INT4 matrix multiply (unpacked)       */

/* ─────────────────────────────────────────────────────────────────────────── */
/* aux_field bit definitions — v2.0 (superset of v0.1)                         */
/* ─────────────────────────────────────────────────────────────────────────── */

/* aux_field[1:0] — precision / cast target (OPC2_CAST, OPC2_MATMUL_*) */
#define AUX2_DTYPE_INT8         0x00u  /* INT8   */
#define AUX2_DTYPE_FP16         0x01u  /* FP16   */
#define AUX2_DTYPE_BF16         0x02u  /* BF16   */
#define AUX2_DTYPE_FP8_E4M3     0x03u  /* FP8 E4M3 */
#define AUX2_DTYPE_FP8_E5M2     0x04u  /* FP8 E5M2 */
#define AUX2_DTYPE_INT4         0x05u  /* INT4   */
#define AUX2_DTYPE_FP32         0x06u  /* FP32 (accumulator read-out only) */

/* aux_field[0] — softmax axis (OPC2_SOFTMAX, unchanged from v0.1) */
#define AUX2_SOFTMAX_AXIS_ROW   0x00u  /* axis = 1 (row-wise) */
#define AUX2_SOFTMAX_AXIS_COL   0x01u  /* axis = 0 (col-wise) */

/* aux_field[0] — causal mask (OPC2_FLASH_ATTN, OPC2_MULTI_HEAD_ATTN) */
#define AUX2_ATTN_BIDIRECTIONAL 0x00u  /* encoder attention — no mask   */
#define AUX2_ATTN_CAUSAL        0x01u  /* decoder attention — causal mask */

/* aux_field[1] — GQA mode (OPC2_MULTI_HEAD_ATTN, OPC2_FLASH_ATTN) */
#define AUX2_ATTN_MHA           0x00u  /* standard multi-head attention */
#define AUX2_ATTN_GQA           0x02u  /* grouped-query attention (LLaMA 3) */

/* aux_field[2] — sparsity (OPC2_SPARSE_MATMUL) */
#define AUX2_SPARSE_NONE        0x00u  /* dense matmul */
#define AUX2_SPARSE_2_4         0x04u  /* 2:4 structured sparsity */

/* aux_field[4:3] — FP8 format pair (OPC2_FP8_CAST, OPC2_MATMUL_FP8) */
#define AUX2_FP8_E4M3_E4M3      0x00u  /* both operands E4M3 */
#define AUX2_FP8_E5M2_E4M3      0x08u  /* activation E5M2, weight E4M3 */
#define AUX2_FP8_E4M3_E5M2      0x10u  /* activation E4M3, weight E5M2 */

/* ─────────────────────────────────────────────────────────────────────────── */
/* Fault codes (halt_status in aux_field when OPC2_HALT + fault)               */
/* ─────────────────────────────────────────────────────────────────────────── */

#define HALT_STATUS_OK          0x00u
#define FAULT_OOB_SRAM          0xE0u  /* SRAM address out of bounds            */
#define FAULT_OOB_HOST          0xE1u  /* Host address out of bounds            */
#define FAULT_BAD_OPCODE        0xE2u  /* Unrecognised opcode                   */
#define FAULT_DTYPE_MISMATCH    0xE3u  /* Operand dtypes incompatible           */
#define FAULT_OOB_TILE_ID       0xE4u  /* Tile register index >= NUM_TILES      */
#define FAULT_LOOP_OVERFLOW     0xE5u  /* Nested LOOP_BEGIN exceeds stack depth */
#define FAULT_LOOP_UNDERFLOW    0xE6u  /* LOOP_END without matching LOOP_BEGIN  */
#define FAULT_BARRIER_TIMEOUT   0xE7u  /* BARRIER wait exceeded max cycles      */
#define FAULT_DRAM_ECC          0xE8u  /* Uncorrectable DRAM ECC error          */
#define FAULT_CERT_TAMPER       0xE9u  /* Cert register tamper detected         */
#define FAULT_CORE_MASK_ZERO    0xEAu  /* core_mask = 0 on multi-core op        */

/* ─────────────────────────────────────────────────────────────────────────── */
/* Core mask helpers                                                            */
/* ─────────────────────────────────────────────────────────────────────────── */

#define CORE_MASK_ALL           0x00FFu /* All 8 cores                         */
#define CORE_MASK_CORE(n)       (1u << (n)) /* Single core n (0–7)            */
#define CORE_MASK_SINGLE        0x0001u /* Core 0 only (default for v0.1 ops) */

/* ─────────────────────────────────────────────────────────────────────────── */
/* Default tile dimensions for 256×256 systolic array                          */
/* ─────────────────────────────────────────────────────────────────────────── */

#define SATIN_DEFAULT_TILE_M    256u
#define SATIN_DEFAULT_TILE_N    256u
#define SATIN_DEFAULT_TILE_K    256u

/* Maximum dimensions */
#define SATIN_MAX_TILE_M        4096u
#define SATIN_MAX_TILE_N        4096u
#define SATIN_MAX_TILE_K        4096u
#define SATIN_MAX_HEAD_COUNT    64u
#define SATIN_MAX_SEQ_LEN       8192u

/* ─────────────────────────────────────────────────────────────────────────── */
/* Packet struct                                                                */
/* ─────────────────────────────────────────────────────────────────────────── */

typedef struct {
    uint8_t raw[SATIN_INSTR_BYTES_V2];   /* 32 bytes, big-endian */
} isa_packet_v2_t;

/* ─────────────────────────────────────────────────────────────────────────── */
/* Field GET macros                                                             */
/* ─────────────────────────────────────────────────────────────────────────── */

#define ISA2_GET_OPCODE(p)       ((p)->raw[IOFF2_OPCODE])
#define ISA2_GET_DST(p)          ((p)->raw[IOFF2_DST_TILE_ID])
#define ISA2_GET_SRC_A(p)        ((p)->raw[IOFF2_SRC_A_ID])
#define ISA2_GET_SRC_B(p)        ((p)->raw[IOFF2_SRC_B_ID])
#define ISA2_GET_SRC_C(p)        ((p)->raw[IOFF2_SRC_C_ID])
#define ISA2_GET_AUX(p)          ((p)->raw[IOFF2_AUX_FIELD])

#define ISA2_GET_SCALAR_IMM(p) \
    ((uint16_t)(((p)->raw[IOFF2_SCALAR_IMM_HI] << 8) | \
                 (p)->raw[IOFF2_SCALAR_IMM_LO]))

#define ISA2_GET_HOST_ADDR(p) \
    ((uint32_t)(((p)->raw[IOFF2_HOST_ADDR_B3] << 24) | \
                ((p)->raw[IOFF2_HOST_ADDR_B2] << 16) | \
                ((p)->raw[IOFF2_HOST_ADDR_B1] <<  8) | \
                 (p)->raw[IOFF2_HOST_ADDR_B0]))

#define ISA2_GET_SRAM_ADDR(p) \
    ((uint32_t)(((p)->raw[IOFF2_SRAM_ADDR_B3] << 24) | \
                ((p)->raw[IOFF2_SRAM_ADDR_B2] << 16) | \
                ((p)->raw[IOFF2_SRAM_ADDR_B1] <<  8) | \
                 (p)->raw[IOFF2_SRAM_ADDR_B0]))

#define ISA2_GET_BYTE_COUNT(p) \
    ((uint32_t)(((p)->raw[IOFF2_BYTE_COUNT_B3] << 24) | \
                ((p)->raw[IOFF2_BYTE_COUNT_B2] << 16) | \
                ((p)->raw[IOFF2_BYTE_COUNT_B1] <<  8) | \
                 (p)->raw[IOFF2_BYTE_COUNT_B0]))

#define ISA2_GET_TILE_M(p) \
    ((uint16_t)(((p)->raw[IOFF2_TILE_M_HI] << 8) | (p)->raw[IOFF2_TILE_M_LO]))

#define ISA2_GET_TILE_N(p) \
    ((uint16_t)(((p)->raw[IOFF2_TILE_N_HI] << 8) | (p)->raw[IOFF2_TILE_N_LO]))

#define ISA2_GET_TILE_K(p) \
    ((uint16_t)(((p)->raw[IOFF2_TILE_K_HI] << 8) | (p)->raw[IOFF2_TILE_K_LO]))

#define ISA2_GET_HEAD_COUNT(p) \
    ((uint16_t)(((p)->raw[IOFF2_HEAD_COUNT_HI] << 8) | (p)->raw[IOFF2_HEAD_COUNT_LO]))

#define ISA2_GET_CORE_MASK(p) \
    ((uint16_t)(((p)->raw[IOFF2_CORE_MASK_HI] << 8) | (p)->raw[IOFF2_CORE_MASK_LO]))

/* ─────────────────────────────────────────────────────────────────────────── */
/* Field SET macros                                                             */
/* ─────────────────────────────────────────────────────────────────────────── */

#define ISA2_SET_OPCODE(p, v)    ((p)->raw[IOFF2_OPCODE]       = (uint8_t)(v))
#define ISA2_SET_DST(p, v)       ((p)->raw[IOFF2_DST_TILE_ID]  = (uint8_t)(v))
#define ISA2_SET_SRC_A(p, v)     ((p)->raw[IOFF2_SRC_A_ID]     = (uint8_t)(v))
#define ISA2_SET_SRC_B(p, v)     ((p)->raw[IOFF2_SRC_B_ID]     = (uint8_t)(v))
#define ISA2_SET_SRC_C(p, v)     ((p)->raw[IOFF2_SRC_C_ID]     = (uint8_t)(v))
#define ISA2_SET_AUX(p, v)       ((p)->raw[IOFF2_AUX_FIELD]    = (uint8_t)(v))

#define ISA2_SET_SCALAR_IMM(p, v) do { \
    (p)->raw[IOFF2_SCALAR_IMM_HI] = (uint8_t)(((v) >> 8) & 0xFF); \
    (p)->raw[IOFF2_SCALAR_IMM_LO] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_HOST_ADDR(p, v) do { \
    (p)->raw[IOFF2_HOST_ADDR_B3] = (uint8_t)(((v) >> 24) & 0xFF); \
    (p)->raw[IOFF2_HOST_ADDR_B2] = (uint8_t)(((v) >> 16) & 0xFF); \
    (p)->raw[IOFF2_HOST_ADDR_B1] = (uint8_t)(((v) >>  8) & 0xFF); \
    (p)->raw[IOFF2_HOST_ADDR_B0] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_SRAM_ADDR(p, v) do { \
    (p)->raw[IOFF2_SRAM_ADDR_B3] = (uint8_t)(((v) >> 24) & 0xFF); \
    (p)->raw[IOFF2_SRAM_ADDR_B2] = (uint8_t)(((v) >> 16) & 0xFF); \
    (p)->raw[IOFF2_SRAM_ADDR_B1] = (uint8_t)(((v) >>  8) & 0xFF); \
    (p)->raw[IOFF2_SRAM_ADDR_B0] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_BYTE_COUNT(p, v) do { \
    (p)->raw[IOFF2_BYTE_COUNT_B3] = (uint8_t)(((v) >> 24) & 0xFF); \
    (p)->raw[IOFF2_BYTE_COUNT_B2] = (uint8_t)(((v) >> 16) & 0xFF); \
    (p)->raw[IOFF2_BYTE_COUNT_B1] = (uint8_t)(((v) >>  8) & 0xFF); \
    (p)->raw[IOFF2_BYTE_COUNT_B0] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_TILE_M(p, v) do { \
    (p)->raw[IOFF2_TILE_M_HI] = (uint8_t)(((v) >> 8) & 0xFF); \
    (p)->raw[IOFF2_TILE_M_LO] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_TILE_N(p, v) do { \
    (p)->raw[IOFF2_TILE_N_HI] = (uint8_t)(((v) >> 8) & 0xFF); \
    (p)->raw[IOFF2_TILE_N_LO] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_TILE_K(p, v) do { \
    (p)->raw[IOFF2_TILE_K_HI] = (uint8_t)(((v) >> 8) & 0xFF); \
    (p)->raw[IOFF2_TILE_K_LO] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_HEAD_COUNT(p, v) do { \
    (p)->raw[IOFF2_HEAD_COUNT_HI] = (uint8_t)(((v) >> 8) & 0xFF); \
    (p)->raw[IOFF2_HEAD_COUNT_LO] = (uint8_t)((v) & 0xFF); \
} while (0)

#define ISA2_SET_CORE_MASK(p, v) do { \
    (p)->raw[IOFF2_CORE_MASK_HI] = (uint8_t)(((v) >> 8) & 0xFF); \
    (p)->raw[IOFF2_CORE_MASK_LO] = (uint8_t)((v) & 0xFF); \
} while (0)

/* ─────────────────────────────────────────────────────────────────────────── */
/* isa_pack_v2() — pack all fields into an isa_packet_v2_t                     */
/* ─────────────────────────────────────────────────────────────────────────── */

static inline void isa_pack_v2(
    isa_packet_v2_t *p,
    uint8_t   opcode,
    uint8_t   dst_tile_id,
    uint8_t   src_a_id,
    uint8_t   src_b_id,
    uint8_t   src_c_id,
    uint16_t  scalar_imm,
    uint32_t  host_addr,
    uint32_t  sram_addr,
    uint32_t  byte_count,
    uint8_t   aux_field,
    uint16_t  tile_M,
    uint16_t  tile_N,
    uint16_t  tile_K,
    uint16_t  head_count,
    uint16_t  core_mask
) {
    memset(p->raw, 0, SATIN_INSTR_BYTES_V2);
    ISA2_SET_OPCODE(p,      opcode);
    ISA2_SET_DST(p,         dst_tile_id);
    ISA2_SET_SRC_A(p,       src_a_id);
    ISA2_SET_SRC_B(p,       src_b_id);
    ISA2_SET_SRC_C(p,       src_c_id);
    ISA2_SET_SCALAR_IMM(p,  scalar_imm);
    ISA2_SET_HOST_ADDR(p,   host_addr);
    ISA2_SET_SRAM_ADDR(p,   sram_addr);
    ISA2_SET_BYTE_COUNT(p,  byte_count);
    ISA2_SET_AUX(p,         aux_field);
    ISA2_SET_TILE_M(p,      tile_M);
    ISA2_SET_TILE_N(p,      tile_N);
    ISA2_SET_TILE_K(p,      tile_K);
    ISA2_SET_HEAD_COUNT(p,  head_count);
    ISA2_SET_CORE_MASK(p,   core_mask);
    /* reserved bytes [30:31] already zero from memset */
}

/*
 * isa_pack_v2_simple() — simplified pack for compute ops (no DMA fields)
 * Most compute instructions only need opcode, tile IDs, aux, tile dims.
 */
static inline void isa_pack_v2_simple(
    isa_packet_v2_t *p,
    uint8_t  opcode,
    uint8_t  dst,
    uint8_t  src_a,
    uint8_t  src_b,
    uint8_t  src_c,
    uint8_t  aux,
    uint16_t tile_M,
    uint16_t tile_N,
    uint16_t tile_K,
    uint16_t core_mask
) {
    isa_pack_v2(p, opcode, dst, src_a, src_b, src_c,
                0x0000, 0, 0, 0,
                aux, tile_M, tile_N, tile_K, 0, core_mask);
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* isa_validate_v2() — validate a packed instruction                            */
/* Returns 0 on valid, negative error code on failure.                          */
/* ─────────────────────────────────────────────────────────────────────────── */

static inline int isa_validate_v2(const isa_packet_v2_t *p) {
    uint8_t opc = ISA2_GET_OPCODE(p);

    /* Reserved bytes must be zero */
    if (p->raw[IOFF2_RESERVED_HI] != 0x00u) return -1;
    if (p->raw[IOFF2_RESERVED_LO] != 0x00u) return -2;

    /* Check opcode is known */
    switch (opc) {
        /* v0.1 opcodes */
        case OPC2_NOP:           case OPC2_DMA_LOAD:      case OPC2_DMA_STORE:
        case OPC2_TILE_LOAD:     case OPC2_TILE_STORE:
        case OPC2_MATMUL_I8:     case OPC2_MATMUL_F16:    case OPC2_TRANSPOSE:
        case OPC2_SCALE:         case OPC2_RELU:           case OPC2_GELU:
        case OPC2_SOFTMAX:       case OPC2_LAYERNORM:
        case OPC2_ELEM_ADD:      case OPC2_ELEM_MUL:
        case OPC2_CAST:          case OPC2_QUANTIZE:       case OPC2_DEQUANTIZE:
        case OPC2_ATTENTION:     case OPC2_TILE_ZERO:      case OPC2_HALT:
        /* v2.0 opcodes */
        case OPC2_FLASH_ATTN:    case OPC2_KV_APPEND:     case OPC2_EMBED_LOOKUP:
        case OPC2_GATHER:        case OPC2_SCATTER:        case OPC2_ROPE_ENCODE:
        case OPC2_BARRIER:       case OPC2_LOOP_BEGIN:     case OPC2_LOOP_END:
        case OPC2_BF16_CAST:     case OPC2_SPARSE_MATMUL:  case OPC2_FP8_CAST:
        case OPC2_INT4_PACK:     case OPC2_INT4_UNPACK:   case OPC2_CERT_HASH:
        case OPC2_MULTI_HEAD_ATTN: case OPC2_PREFETCH:
        case OPC2_MATMUL_BF16:   case OPC2_MATMUL_FP8:    case OPC2_MATMUL_I4:
            break;
        default:
            return -3;  /* FAULT_BAD_OPCODE */
    }

    /* CAST dtype must be a known value */
    if (opc == OPC2_CAST || opc == OPC2_BF16_CAST || opc == OPC2_FP8_CAST) {
        uint8_t dtype = ISA2_GET_AUX(p) & 0x07u;
        if (dtype > AUX2_DTYPE_FP32) return -4;
    }

    /* Multi-core ops must have non-zero core_mask */
    if (opc == OPC2_MULTI_HEAD_ATTN || opc == OPC2_FLASH_ATTN ||
        opc == OPC2_SPARSE_MATMUL   || opc == OPC2_MATMUL_BF16 ||
        opc == OPC2_MATMUL_FP8      || opc == OPC2_MATMUL_I4) {
        if (ISA2_GET_CORE_MASK(p) == 0x0000u) return -5;  /* FAULT_CORE_MASK_ZERO */
    }

    /* tile_M, tile_N, tile_K must be <= SATIN_MAX_TILE_* when non-zero */
    uint16_t tM = ISA2_GET_TILE_M(p);
    uint16_t tN = ISA2_GET_TILE_N(p);
    uint16_t tK = ISA2_GET_TILE_K(p);
    if (tM != 0 && tM > SATIN_MAX_TILE_M) return -6;
    if (tN != 0 && tN > SATIN_MAX_TILE_N) return -7;
    if (tK != 0 && tK > SATIN_MAX_TILE_K) return -8;

    /* head_count must be <= SATIN_MAX_HEAD_COUNT when non-zero */
    uint16_t hc = ISA2_GET_HEAD_COUNT(p);
    if (hc != 0 && hc > SATIN_MAX_HEAD_COUNT) return -9;

    return 0;  /* valid */
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Convenience: upgrade a v0.1 packet to v2.0                                  */
/* Copies 16-byte v0.1 raw data into the upper half of a v2.0 packet,         */
/* then fills new fields with safe defaults.                                    */
/* ─────────────────────────────────────────────────────────────────────────── */

static inline void isa_upgrade_v1_to_v2(
    isa_packet_v2_t       *dst,
    const uint8_t          src_v1[16]
) {
    memset(dst->raw, 0, SATIN_INSTR_BYTES_V2);

    /* Copy v0.1 fields: opcode, dst, src_a, src_b, src_c, scalar_imm */
    dst->raw[IOFF2_OPCODE]       = src_v1[0];
    dst->raw[IOFF2_DST_TILE_ID]  = src_v1[1];
    dst->raw[IOFF2_SRC_A_ID]     = src_v1[2];
    dst->raw[IOFF2_SRC_B_ID]     = src_v1[3];
    dst->raw[IOFF2_SRC_C_ID]     = src_v1[4];
    dst->raw[IOFF2_SCALAR_IMM_HI]= src_v1[5];
    dst->raw[IOFF2_SCALAR_IMM_LO]= src_v1[6];

    /* v0.1 host_addr [15:0] → v2.0 host_addr [31:0] (zero-extend, big-endian) */
    dst->raw[IOFF2_HOST_ADDR_B3] = 0;
    dst->raw[IOFF2_HOST_ADDR_B2] = 0;
    dst->raw[IOFF2_HOST_ADDR_B1] = src_v1[7];
    dst->raw[IOFF2_HOST_ADDR_B0] = src_v1[8];

    /* v0.1 sram_addr [15:0] → v2.0 sram_addr [31:0] (zero-extend) */
    dst->raw[IOFF2_SRAM_ADDR_B3] = 0;
    dst->raw[IOFF2_SRAM_ADDR_B2] = 0;
    dst->raw[IOFF2_SRAM_ADDR_B1] = src_v1[9];
    dst->raw[IOFF2_SRAM_ADDR_B0] = src_v1[10];

    /* v0.1 byte_count [15:0] → v2.0 byte_count [31:0] (zero-extend) */
    dst->raw[IOFF2_BYTE_COUNT_B3] = 0;
    dst->raw[IOFF2_BYTE_COUNT_B2] = 0;
    dst->raw[IOFF2_BYTE_COUNT_B1] = src_v1[11];
    dst->raw[IOFF2_BYTE_COUNT_B0] = src_v1[12];

    /* v0.1 aux_field */
    dst->raw[IOFF2_AUX_FIELD]    = src_v1[13];

    /* New v2.0 fields: safe defaults */
    ISA2_SET_TILE_M(dst,    SATIN_DEFAULT_TILE_M);
    ISA2_SET_TILE_N(dst,    SATIN_DEFAULT_TILE_N);
    ISA2_SET_TILE_K(dst,    SATIN_DEFAULT_TILE_K);
    ISA2_SET_HEAD_COUNT(dst, 0);
    ISA2_SET_CORE_MASK(dst, CORE_MASK_SINGLE);  /* v0.1 ops run on core 0 only */
}

/* ─────────────────────────────────────────────────────────────────────────── */
/* Usage examples                                                               */
/* ─────────────────────────────────────────────────────────────────────────── */

/*
 * Example 1 — 256×256 BF16 matmul on all 8 cores:
 *
 *   isa_packet_v2_t pkt;
 *   isa_pack_v2_simple(&pkt, OPC2_MATMUL_BF16,
 *       dst_tile, src_a_tile, src_b_tile, 0xFF,
 *       AUX2_DTYPE_BF16,
 *       256, 256, 256,
 *       CORE_MASK_ALL);
 *
 * Example 2 — Flash Attention, causal, GQA, S=2048, D=128, H=32, 8 cores:
 *
 *   isa_packet_v2_t pkt;
 *   isa_pack_v2(&pkt,
 *       OPC2_FLASH_ATTN,
 *       out_tile, q_tile, k_tile, v_tile,
 *       0x0000,        // scalar_imm (sqrt(D) handled internally)
 *       0, 0, 0,       // host_addr, sram_addr, byte_count (not used)
 *       AUX2_ATTN_CAUSAL | AUX2_ATTN_GQA,
 *       2048,          // tile_M = S
 *       128,           // tile_N = D
 *       32,            // tile_K = H
 *       32,            // head_count
 *       CORE_MASK_ALL);
 *
 * Example 3 — DMA_LOAD 4 MB from host address 0x10000000 to SRAM 0x00000000:
 *
 *   isa_packet_v2_t pkt;
 *   isa_pack_v2(&pkt,
 *       OPC2_DMA_LOAD, 0, 0, 0, 0,
 *       0,
 *       0x10000000u,   // host_addr (32-bit)
 *       0x00000000u,   // sram_addr
 *       4u * 1024u * 1024u,  // byte_count = 4 MB
 *       0, 0, 0, 0, 0,
 *       CORE_MASK_SINGLE);
 *
 * Example 4 — Upgrade a v0.1 program to v2.0:
 *
 *   uint8_t v1_raw[16] = { ... };  // compiled by compiler_v0.py
 *   isa_packet_v2_t v2_pkt;
 *   isa_upgrade_v1_to_v2(&v2_pkt, v1_raw);
 *   // v2_pkt now valid on Gen 3 hardware, runs on core 0 only
 */

#endif /* SATIN_ISA_PACKET_V2_H */
