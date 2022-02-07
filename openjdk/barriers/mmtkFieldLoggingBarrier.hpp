#ifndef MMTK_BARRIERS_FIELD_LOGGING_BARRIER
#define MMTK_BARRIERS_FIELD_LOGGING_BARRIER

#include "opto/callnode.hpp"
#include "opto/idealKit.hpp"
#include "c1/c1_LIRAssembler.hpp"
#include "c1/c1_MacroAssembler.hpp"
#include "gc/shared/barrierSet.hpp"
#include "../mmtk.h"
#include "../mmtkBarrierSet.hpp"
#include "../mmtkBarrierSetAssembler_x86.hpp"
#include "../mmtkBarrierSetC1.hpp"
#include "../mmtkBarrierSetC2.hpp"

#define SIDE_METADATA_WORST_CASE_RATIO_LOG 1
#define LOG_BYTES_IN_CHUNK 22
#define CHUNK_MASK ((1L << LOG_BYTES_IN_CHUNK) - 1)

#define MMTK_ENABLE_BARRIER_FASTPATH true

class MMTkFieldLoggingBarrierSetRuntime: public MMTkBarrierSetRuntime {
public:
  static void record_modified_node_slow(void* src, void* slot, void* val);
  static void record_clone_slow(void* src, void* dst, size_t size);
  static void record_array_copy_slow(void* src, void* dst, const size_t size) {
    record_array_copy_inline(src, dst, size, NULL);
  }

// CHANGED
  inline static void record_array_copy_mmtk_slow(const void* src, const void* dst, const size_t size, void* dst_obj) {
    ::mmtk_object_reference_arraycopy(
      (MMTk_Mutator) &Thread::current()->third_party_heap_mutator,
      (void*) ((intptr_t) src),
      0,
      (void*) ((intptr_t) dst_obj),
      ((intptr_t) dst) - ((intptr_t) dst_obj),
      size
    );
  }

//CHANGED
  inline static void record_array_copy_inline(const void* src_ptr, void* dst_ptr, const size_t size, void* dst_obj) {
    //if (size == 0) return;
    //if (size == 1) {
     // intptr_t addr = (intptr_t) dst_ptr;
      //uint8_t* meta_addr = (uint8_t*) (SIDE_METADATA_BASE_ADDRESS + (addr >> 6));
      //intptr_t shift = (addr >> 3) & 0b111;
      //uint8_t byte_val = *meta_addr;
      //if (((byte_val >> shift) & 1) != 0) {
      //  record_modified_node_slow(dst_obj, dst_ptr, (void*) NULL);
      //}
      //return;
    //}

    record_array_copy_mmtk_slow(src_ptr, dst_ptr, size, dst_obj);

    // constexpr intptr_t kLogBitsInUInt = 6;
    // constexpr intptr_t kBitsInUInt = 1 << kLogBitsInUInt;
    // constexpr intptr_t kBytesInUInt = 8;
    // intptr_t origin_dst = (intptr_t) dst_ptr;
    // intptr_t dst = origin_dst;
    // intptr_t src = (intptr_t) src_ptr;
    // intptr_t limit = dst + (size << 3);
    // auto meta_addr = (uint64_t*) ((SIDE_METADATA_BASE_ADDRESS + (dst >> 6)) >> 3 << 3);
    // auto val = *meta_addr;
    // while (dst < limit) {
    //   if ((dst & 0b111111111) == 0) {
    //     auto val = *meta_addr;
    //     for (auto j = 0; j < kBitsInUInt; j++) {
    //         if (((val >> j) & 1) != 0) {
    //             auto e = dst + (j << 3);
    //             if (e < limit) {
    //               auto offset = e - origin_dst;
    //               record_array_copy_mmtk_slow((void*) (src + offset), (void*) e, size - (offset >> 3));
    //               return;
    //             }
    //         }
    //     }
    //     dst += 1 << (kLogBitsInUInt + 3);
    //     meta_addr += kBytesInUInt;
    //   } else {
    //     auto shift = (dst >> 3) & (kBitsInUInt - 1);
    //     if ((val >> shift) & 1 != 0) {
    //       auto offset = dst - origin_dst;
    //       record_array_copy_mmtk_slow((void*) (src + offset), (void*) dst, size - (offset >> 3));
    //       return;
    //     }
    //     dst += kBytesInUInt;
    //   }
    // }

    // const auto origin_dst = dst;
    // size_t i = 0;
    // {
    //   const uint64_t* meta_addr = (uint64_t*) ((SIDE_METADATA_BASE_ADDRESS + (((intptr_t) dst) >> 6)) >> 3 << 3);
    //   const uint64_t val = *meta_addr;
    //   if (val != 0) {
    //     const auto shift_base = ((((intptr_t) dst) >> 3) & 0b111111);
    //     while (i < size && (((intptr_t) dst) & 0b111111111) != 0) {
    //       const intptr_t shift = shift_base + i;
    //       if (((val >> shift) & 1) != 0) {
    //         ::mmtk_object_reference_arraycopy(
    //           (MMTk_Mutator) &Thread::current()->third_party_heap_mutator,
    //           (void*) (((intptr_t) src) + (i << 3)),
    //           0,
    //           (void*) (((intptr_t) origin_dst) + (i << 3)),
    //           0,
    //           size - i
    //         );
    //         return;
    //       }
    //       i++;
    //       dst = (void*) (((intptr_t) dst) + 8);
    //     }
    //   } else {
    //     const auto new_dst = (void*) ((((intptr_t) dst) + 0b111111111L) >> 9 << 9);
    //     const auto dist = ((intptr_t) new_dst) - ((intptr_t) dst);
    //     i = i + (dist >> 3);
    //     dst = new_dst;
    //   }
    // }
    // if (i >= size) return;
    // uint64_t* meta_addr = (uint64_t*) (SIDE_METADATA_BASE_ADDRESS + (((intptr_t) dst) >> 6));
    // for (; i < size;) {
    //   if (*meta_addr != 0) {
    //     ::mmtk_object_reference_arraycopy(
    //       (MMTk_Mutator) &Thread::current()->third_party_heap_mutator,
    //       (void*) (((intptr_t) src) + (i << 3)),
    //       0,
    //       (void*) (((intptr_t) origin_dst) + (i << 3)),
    //       0,
    //       size - i
    //     );
    //     return;
    //   }
    //   i += 64;
    //   meta_addr += 1;
    // }
  }

  virtual bool is_slow_path_call(address call) {
    return call == CAST_FROM_FN_PTR(address, record_modified_node_slow) || call == CAST_FROM_FN_PTR(address, record_clone_slow) || call == CAST_FROM_FN_PTR(address, record_array_copy_slow);
  }

  virtual void record_modified_node(oop src, ptrdiff_t offset, oop val);
  virtual void record_clone(oop src, oop dst, size_t size);
  virtual void record_arraycopy(arrayOop src_obj, size_t src_offset_in_bytes, oop* src_raw, arrayOop dst_obj, size_t dst_offset_in_bytes, oop* dst_raw, size_t length);
};

class MMTkFieldLoggingBarrierSetC1;
class MMTkFieldLoggingBarrierStub;

class MMTkFieldLoggingBarrierSetAssembler: public MMTkBarrierSetAssembler {
  void oop_store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type, Address dst, Register val, Register tmp1, Register tmp2);
  void record_modified_node(MacroAssembler* masm, Address dst, Register val, Register tmp1, Register tmp2);
public:
  virtual void store_at(MacroAssembler* masm, DecoratorSet decorators, BasicType type, Address dst, Register val, Register tmp1, Register tmp2) {
    if (type == T_OBJECT || type == T_ARRAY) {
      oop_store_at(masm, decorators, type, dst, val, tmp1, tmp2);
    } else {
      BarrierSetAssembler::store_at(masm, decorators, type, dst, val, tmp1, tmp2);
    }
  }
  virtual void arraycopy_prologue(MacroAssembler* masm, DecoratorSet decorators, BasicType type, Register src, Register dst, Register count) override;
  // virtual void arraycopy_prologue2(MacroAssembler* masm, DecoratorSet decorators, BasicType type, Register src, Register dst, Register count, Register dst_obj) override;
  inline void gen_write_barrier_stub(LIR_Assembler* ce, MMTkFieldLoggingBarrierStub* stub);
#define __ sasm->
  void generate_c1_write_barrier_runtime_stub(StubAssembler* sasm) {
    __ prologue("mmtk_write_barrier", false);

    Address store_addr(rbp, 3*BytesPerWord);

    Label done;
    Label runtime;

    __ push(c_rarg0);
    // __ push(c_rarg1);
    // __ push(c_rarg2);
    __ push(rax);

    __ load_parameter(0, c_rarg0);
    // __ load_parameter(1, c_rarg1);
    // __ load_parameter(2, c_rarg2);

    __ bind(runtime);

    __ save_live_registers_no_oop_map(true);

    __ call_VM_leaf_base(CAST_FROM_FN_PTR(address, MMTkFieldLoggingBarrierSetRuntime::record_modified_node_slow), 1);

    __ restore_live_registers(true);

    __ bind(done);
    __ pop(rax);
    // __ pop(c_rarg2);
    // __ pop(c_rarg1);
    __ pop(c_rarg0);

    __ epilogue();
  }
#undef __
};

#ifdef ASSERT
#define __ gen->lir(__FILE__, __LINE__)->
#else
#define __ gen->lir()->
#endif

struct MMTkFieldLoggingBarrierStub: CodeStub {
  LIR_Opr _src, _slot, _new_val;
  MMTkFieldLoggingBarrierStub(LIR_Opr src, LIR_Opr slot, LIR_Opr new_val): _src(src), _slot(slot), _new_val(new_val) {}
  virtual void emit_code(LIR_Assembler* ce) {
    MMTkFieldLoggingBarrierSetAssembler* bs = (MMTkFieldLoggingBarrierSetAssembler*) BarrierSet::barrier_set()->barrier_set_assembler();
    bs->gen_write_barrier_stub(ce, this);
  }
  virtual void visit(LIR_OpVisitState* visitor) {
    visitor->do_slow_case();
    if (_src != NULL) visitor->do_input(_src);
    if (_slot != NULL) visitor->do_input(_slot);
    if (_new_val != NULL) visitor->do_input(_new_val);
  }
  NOT_PRODUCT(virtual void print_name(outputStream* out) const { out->print("MMTkWriteBarrierStub"); });
};

class MMTkFieldLoggingBarrierSetC1: public MMTkBarrierSetC1 {
public:
  class MMTkFieldLoggingBarrierCodeGenClosure : public StubAssemblerCodeGenClosure {
    virtual OopMapSet* generate_code(StubAssembler* sasm) {
      MMTkFieldLoggingBarrierSetAssembler* bs = (MMTkFieldLoggingBarrierSetAssembler*) BarrierSet::barrier_set()->barrier_set_assembler();
      bs->generate_c1_write_barrier_runtime_stub(sasm);
      return NULL;
    }
  };
  void record_modified_node(LIRAccess& access, LIR_Opr src, LIR_Opr slot, LIR_Opr new_val);
public:
  CodeBlob* _write_barrier_c1_runtime_code_blob;
  virtual void store_at_resolved(LIRAccess& access, LIR_Opr value) {
    BarrierSetC1::store_at_resolved(access, value);
    if (access.is_oop()) record_modified_node(access, access.base().opr(), access.resolved_addr(), value);
  }
  virtual LIR_Opr atomic_cmpxchg_at_resolved(LIRAccess& access, LIRItem& cmp_value, LIRItem& new_value) {
    LIR_Opr result = BarrierSetC1::atomic_cmpxchg_at_resolved(access, cmp_value, new_value);
    if (access.is_oop()) record_modified_node(access, access.base().opr(), access.resolved_addr(), new_value.result());
    return result;
  }
  virtual LIR_Opr atomic_xchg_at_resolved(LIRAccess& access, LIRItem& value) {
    LIR_Opr result = BarrierSetC1::atomic_xchg_at_resolved(access, value);
    if (access.is_oop()) record_modified_node(access, access.base().opr(), access.resolved_addr(), value.result());
    return result;
  }
  virtual void generate_c1_runtime_stubs(BufferBlob* buffer_blob) {
    MMTkFieldLoggingBarrierCodeGenClosure write_code_gen_cl;
    _write_barrier_c1_runtime_code_blob = Runtime1::generate_blob(buffer_blob, -1, "write_code_gen_cl", false, &write_code_gen_cl);
  }
  virtual LIR_Opr resolve_address(LIRAccess& access, bool resolve_in_register) {
    DecoratorSet decorators = access.decorators();
    bool needs_patching = (decorators & C1_NEEDS_PATCHING) != 0;
    bool is_write = (decorators & C1_WRITE_ACCESS) != 0;
    bool is_array = (decorators & IS_ARRAY) != 0;
    bool on_anonymous = (decorators & ON_UNKNOWN_OOP_REF) != 0;
    bool precise = is_array || on_anonymous;
    resolve_in_register |= !needs_patching && is_write && access.is_oop() && precise;
    return BarrierSetC1::resolve_address(access, resolve_in_register);
  }
};

#undef __

#define __ ideal.

class MMTkFieldLoggingBarrierSetC2: public MMTkBarrierSetC2 {
  void record_modified_node(GraphKit* kit, Node* node, Node* slot, Node* val) const;
  void record_clone(GraphKit* kit, Node* src, Node* dst, Node* size) const;
  bool can_remove_barrier(GraphKit* kit, PhaseTransform* phase, Node* adr) const;
public:
  virtual Node* store_at_resolved(C2Access& access, C2AccessValue& val) const {
    if (access.is_oop()) record_modified_node(access.kit(), access.base(), access.addr().node(), val.node());
    Node* store = BarrierSetC2::store_at_resolved(access, val);
    return store;
  }
  virtual Node* atomic_cmpxchg_val_at_resolved(C2AtomicAccess& access, Node* expected_val, Node* new_val, const Type* value_type) const {
    if (access.is_oop()) record_modified_node(access.kit(), access.base(), access.addr().node(), new_val);
    Node* result = BarrierSetC2::atomic_cmpxchg_val_at_resolved(access, expected_val, new_val, value_type);
    return result;
  }
  virtual Node* atomic_cmpxchg_bool_at_resolved(C2AtomicAccess& access, Node* expected_val, Node* new_val, const Type* value_type) const {
    if (access.is_oop()) record_modified_node(access.kit(), access.base(), access.addr().node(), new_val);
    Node* load_store = BarrierSetC2::atomic_cmpxchg_bool_at_resolved(access, expected_val, new_val, value_type);
    return load_store;
  }
  virtual Node* atomic_xchg_at_resolved(C2AtomicAccess& access, Node* new_val, const Type* value_type) const {
    if (access.is_oop()) record_modified_node(access.kit(), access.base(), access.addr().node(), new_val);
    Node* result = BarrierSetC2::atomic_xchg_at_resolved(access, new_val, value_type);
    return result;
  }
  virtual void clone(GraphKit* kit, Node* src, Node* dst, Node* size, bool is_array) const {
    record_clone(kit, src, dst, size);
    BarrierSetC2::clone(kit, src, dst, size, is_array);
  }
  virtual bool is_gc_barrier_node(Node* node) const {
    if (node->Opcode() != Op_CallLeaf) return false;
    CallLeafNode *call = node->as_CallLeaf();
    return call->_name != NULL && (strcmp(call->_name, "record_modified_node") == 0 || strcmp(call->_name, "record_clone") == 0);
  }
  virtual bool array_copy_requires_gc_barriers(BasicType type) const override {
    return false;
  }
};

#undef __

#define __ ce->masm()->
inline void MMTkFieldLoggingBarrierSetAssembler::gen_write_barrier_stub(LIR_Assembler* ce, MMTkFieldLoggingBarrierStub* stub) {
  MMTkFieldLoggingBarrierSetC1* bs = (MMTkFieldLoggingBarrierSetC1*) BarrierSet::barrier_set()->barrier_set_c1();
  __ bind(*stub->entry());
  ce->store_parameter(stub->_src->as_pointer_register(), 0);
  __ call(RuntimeAddress(bs->_write_barrier_c1_runtime_code_blob->code_begin()));
  __ jmp(*stub->continuation());
}
#undef __

struct MMTkFieldLoggingBarrier: MMTkBarrierImpl<
  MMTkFieldLoggingBarrierSetRuntime,
  MMTkFieldLoggingBarrierSetAssembler,
  MMTkFieldLoggingBarrierSetC1,
  MMTkFieldLoggingBarrierSetC2
> {};

#endif