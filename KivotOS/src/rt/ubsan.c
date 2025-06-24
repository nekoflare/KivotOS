//
// Original by nyaux.
// https://github.com/rayanmargham/NyauxKC/blob/master/kernel/src/mem/sanitizers/UBsan/ubsan.c
//

#define ANSI_RED "\033[31m"
#define ANSI_RESET "\033[0m"

#include <stdint.h>
#include <string.h>
#include <x86/log.h>

#define is_aligned(value, alignment) !(value & (alignment - 1))

typedef struct {
  const char *filename;
  uint32_t line;
  uint32_t column;
} source_location_t;

typedef struct {
  uint16_t kind;
  uint16_t info;
  char name[];
} type_descriptor_t;

typedef struct {
  source_location_t location;
} data_only_location_t;

typedef struct {
  source_location_t location;
  type_descriptor_t *type;
} data_location_type_t;

typedef struct {
  source_location_t location;
  type_descriptor_t *type;
} data_load_invalid_value_t;

typedef struct {
  source_location_t location;
  source_location_t attr_location;
  int arg_index;
} data_nonnull_arg_t;

typedef struct {
  source_location_t location;
  type_descriptor_t *lhs_type;
  type_descriptor_t *rhs_type;
} data_shift_out_of_bounds_t;

typedef struct {
  source_location_t location;
  type_descriptor_t *array_type;
  type_descriptor_t *index_type;
} data_out_of_bounds_t;

typedef struct {
  source_location_t location;
  type_descriptor_t *type;
  uint8_t alignment;
  uint8_t type_check_kind;
} data_type_mismatch_t;

typedef struct {
  source_location_t location;
  source_location_t assumption_location;
  type_descriptor_t *type;
} data_alignment_assumption_t;

typedef struct {
  source_location_t location;
  type_descriptor_t *from_type;
  type_descriptor_t *to_type;
} data_float_cast_overflow_t;

typedef struct {
  source_location_t location;
  type_descriptor_t *from_type;
  type_descriptor_t *to_type;
  uint8_t kind;
} data_implicit_conversion_t;

typedef struct {
  source_location_t location;
  uint8_t kind;
} data_invalid_builtin_t;
struct ubsan_function_type_mismatch_data
{
	source_location_t location;
	struct type_descriptor_t* type;
} ;
const char *kind_to_type(uint16_t kind) {
  const char *type;
  switch (kind) {
  case 0:
    type = "integer";
    break;
  case 1:
    type = "float";
    break;
  default:
    type = "unknown";
    break;
  }
  return type;
}

unsigned int info_to_bits(uint16_t info) { return 1 << (info >> 1); }
void __ubsan_handle_load_invalid_value(data_load_invalid_value_t *data,
                                       uintptr_t value) {
  debug_print(
    ANSI_RED "UBSAN: load_invalid_value @ %s:%u:%u {value: %#lx}" ANSI_RESET
    "\n",
    data->location.filename, data->location.line, data->location.column,
          value);
}
void __ubsan_handle_nonnull_arg(data_nonnull_arg_t *data) {
  debug_print(
    ANSI_RED "UBSAN: handle_nonnull_arg @ %s:%u:%u {arg_index: %i}" ANSI_RESET
    "\n",
    data->location.filename, data->location.line, data->location.column,
          data->arg_index);
}
void __ubsan_handle_nullability_arg(data_nonnull_arg_t *data) {
  debug_print(
    ANSI_RED "UBSAN: nullability_arg @ %s:%u:%u {arg_index: %i}" ANSI_RESET
    "\n",
    data->location.filename, data->location.line, data->location.column,
          data->arg_index);
}
void __ubsan_handle_nonnull_return_v1(data_only_location_t *data
                                      [[maybe_unused]],
                                      source_location_t *location) {
  debug_print(ANSI_RED "UBSAN: nonnull_return @ %s:%u:%u" ANSI_RESET "\n",
              location->filename,
              location->line, location->column);
}
void __ubsan_handle_nullability_return_v1(data_only_location_t *data
                                          [[maybe_unused]],
                                          source_location_t *location) {
  debug_print(ANSI_RED "UBSAN: nullability_return @ %s:%u:%u" ANSI_RESET "\n",
              location->filename,
              location->line, location->column);
}
void __ubsan_handle_vla_bound_not_positive(data_location_type_t *data,
                                           uintptr_t bound) {
  debug_print(
    ANSI_RED
    "UBSAN: vla_bound_not_positive @ %s:%u:%u {bound: %#lx, type: %u-bit "
          "%s %s}\n",
          data->location.filename, data->location.line, data->location.column,
          bound, info_to_bits(data->type->info), kind_to_type(data->type->kind),
          data->type->name);
}
void __ubsan_handle_add_overflow(data_location_type_t *data, uintptr_t lhs,
                                 uintptr_t rhs) {
  debug_print(
    ANSI_RED
    "UBSAN: add_overflow @ %s:%u:%u {lhs: %#lx, rhs: %#lx, type: %u-bit "
          "%s %s}\n",
          data->location.filename, data->location.line, data->location.column,
          lhs, rhs, info_to_bits(data->type->info),
          kind_to_type(data->type->kind), data->type->name);
}
void __ubsan_handle_sub_overflow(data_location_type_t *data, uintptr_t lhs,
                                 uintptr_t rhs) {
  debug_print(
    ANSI_RED
    "UBSAN: sub_overflow @ %s:%u:%u {lhs: %#lx, rhs: %#lx, type: %u-bit "
          "%s %s}\n",
          data->location.filename, data->location.line, data->location.column,
          lhs, rhs, info_to_bits(data->type->info),
          kind_to_type(data->type->kind), data->type->name);
}



void __ubsan_handle_mul_overflow(data_location_type_t *data, uintptr_t lhs,
                                 uintptr_t rhs) {
  debug_print(
    ANSI_RED
    "UBSAN: mul_overflow @ %s:%u:%u {lhs: %#lx, rhs: %#lx, type: %u-bit "
    "%s %s}" ANSI_RESET "\n",
    data->location.filename, data->location.line, data->location.column,
          lhs, rhs, info_to_bits(data->type->info),
          kind_to_type(data->type->kind), data->type->name);
}
void __ubsan_handle_function_type_mismatch(void* data_raw,
                                           void* value_raw) {
                                            struct ubsan_function_type_mismatch_data
                                                *data =
                                                (struct
                                                  ubsan_function_type_mismatch_data
                                                  *) data_raw;
                                            debug_print(
                                              ANSI_RED
                                              "UBSAN: function type mismatch @ %s:%u:%u"
                                              ANSI_RESET,
                                              data->location.filename,
                                              data->location.line, data->location.column);
}

void __ubsan_handle_divrem_overflow(data_location_type_t *data, uintptr_t lhs,
                                    uintptr_t rhs) {
  debug_print(ANSI_RED "UBSAN: divrem_overflow @ %s:%u:%u {lhs: %#lx, rhs: %#lx, type: "
          "%u-bit %s %s}" ANSI_RESET "\n",
          data->location.filename, data->location.line, data->location.column,
          lhs, rhs, info_to_bits(data->type->info),
          kind_to_type(data->type->kind), data->type->name);
}
void __ubsan_handle_negate_overflow(data_location_type_t *data, uintptr_t old) {
  debug_print(
    ANSI_RED "UBSAN: negate_overflow @ %s:%u:%u {old: %#lx, type: %u-bit %s %s}" ANSI_RESET "\n",
          data->location.filename, data->location.line, data->location.column,
          old, info_to_bits(data->type->info), kind_to_type(data->type->kind),
          data->type->name);
}
void __ubsan_handle_shift_out_of_bounds(data_shift_out_of_bounds_t *data,
                                        uintptr_t lhs, uintptr_t rhs) {
  debug_print(ANSI_RED "UBSAN: shift_out_of_bounds @ %s:%u:%u {lhs: %#lx, rhs: %#lx, "
              "rhs_type: %u-bit %s %s, lhs_type: %u-bit %s %s}" ANSI_RESET "\n",
              data->location.filename, data->location.line, data->location.column,
          lhs, rhs, info_to_bits(data->rhs_type->info),
          kind_to_type(data->rhs_type->kind), data->rhs_type->name,
          info_to_bits(data->lhs_type->info),
          kind_to_type(data->lhs_type->kind), data->lhs_type->name);
}
void __ubsan_handle_out_of_bounds(data_out_of_bounds_t *data, uint64_t index) {
  debug_print(ANSI_RED "UBSAN: out_of_bounds @ %s:%u:%u {index: %#lx, array_type: %u-bit %s "
              "%s, index_type: %u-bit %s %s}" ANSI_RESET "\n",
              data->location.filename, data->location.line, data->location.column,
          index, info_to_bits(data->array_type->info),
          kind_to_type(data->array_type->kind), data->array_type->name,
          info_to_bits(data->index_type->info),
          kind_to_type(data->index_type->kind), data->index_type->name);
}

void __ubsan_handle_type_mismatch(data_type_mismatch_t *data,
                                     void *pointer) {
  static const char *kind_strs[] = {"load of",
                                    "store to",
                                    "reference binding to",
                                    "member access within",
                                    "member call on",
                                    "constructor call on",
                                    "downcast of",
                                    "downcast of",
                                    "upcast of",
                                    "cast to virtual base of",
                                    "nonnull binding to",
                                    "dynamic operation on"};
  if (strcmp(data->location.filename, "src/flanterm/backends/fb.c") == 0) {
    return;
  }
  if (pointer == NULL) {
    debug_print(ANSI_RED "UBSAN: type_mismatch @ %s:%u:%u (%s NULL pointer of type %s)" ANSI_RESET "\n",
                data->location.filename, data->location.line,
                data->location.column,
                kind_strs[data->type_check_kind], data->type->name);
  } else if (data->alignment && !is_aligned((uint64_t)pointer, data->alignment)) {
    debug_print("UBSAN: type_mismatch @ %s:%u:%u (%s misaligned address %#lx of "
            "type %s)\n",
            data->location.filename, data->location.line, data->location.column,
            kind_strs[data->type_check_kind], (uintptr_t)pointer,
            data->type->name);
  } else {
    debug_print(ANSI_RED "UBSAN: type_mismatch @ %s:%u:%u (%s address %#lx not long enough "
            "for type %s)\n",
            data->location.filename, data->location.line, data->location.column,
            kind_strs[data->type_check_kind], (uintptr_t)pointer,
            data->type->name);
  }
}
void __ubsan_handle_type_mismatch_v1(data_type_mismatch_t *data,
                                     void *pointer) {
  static const char *kind_strs[] = {"load of",
                                    "store to",
                                    "reference binding to",
                                    "member access within",
                                    "member call on",
                                    "constructor call on",
                                    "downcast of",
                                    "downcast of",
                                    "upcast of",
                                    "cast to virtual base of",
                                    "nonnull binding to",
                                    "dynamic operation on"
  };
  data->alignment = 1UL << data->alignment;
  if (strcmp(data->location.filename, "src/flanterm/backends/fb.c") == 0) {
    return;
  }
  if (pointer == NULL) {
    debug_print("UBSAN: type_mismatch @ %s:%u:%u (%s NULL pointer of type %s)\n",
            data->location.filename, data->location.line, data->location.column,
            kind_strs[data->type_check_kind], data->type->name);
  } else if (data->alignment && !is_aligned((uint64_t)pointer, data->alignment)) {
    debug_print(ANSI_RED "UBSAN: type_mismatch @ %s:%u:%u (%s misaligned address %#lx of "
            "type %s)\n",
            data->location.filename, data->location.line, data->location.column,
            kind_strs[data->type_check_kind], (uintptr_t) pointer,
            data->type->name);
  } else {
    debug_print("UBSAN: type_mismatch @ %s:%u:%u (%s address %#lx not long enough "
                "for type %s)\n",
                data->location.filename, data->location.line, data->location.
                column,
                kind_strs[data->type_check_kind], (uintptr_t) pointer,
                data->type->name);
  }
}
void __ubsan_handle_alignment_assumption(data_alignment_assumption_t *data,
                                         void *, void *, void *) {
  debug_print(ANSI_RED "UBSAN: alignment_assumption @ %s:%u:%u" ANSI_RESET "\n", data->location.filename,
              data->location.line, data->location.column);
}

void __ubsan_handle_float_cast_overflow(data_float_cast_overflow_t *data,
                                        void *from) {
  debug_print(
    ANSI_RED
    "UBSAN: float_cast_overflow @ %s:%u:%u {from_type: %u-bit %s %s, to_type: %u-bit %s %s}"
    ANSI_RESET "\n",
    data->location.filename, data->location.line, data->location.column,
    info_to_bits(data->from_type->info), kind_to_type(data->from_type->kind),
    data->from_type->name,
    info_to_bits(data->to_type->info), kind_to_type(data->to_type->kind),
    data->to_type->name);
}

void __ubsan_handle_implicit_conversion(data_implicit_conversion_t *data,
                                        void *, void *) {
  debug_print(ANSI_RED "UBSAN: implicit_conversion @ %s:%u:%u" ANSI_RESET "\n", data->location.filename,
              data->location.line, data->location.column);
}
void __ubsan_handle_invalid_builtin(data_invalid_builtin_t *data) {
  debug_print(ANSI_RED "UBSAN: invalid_builtin @ %s:%u:%u" ANSI_RESET "\n", data->location.filename,
              data->location.line, data->location.column);
}
void __ubsan_handle_pointer_overflow(data_only_location_t *data, void *,
                                     void *) {
  debug_print(ANSI_RED "UBSAN: pointer_overflow @ %s:%u:%u" ANSI_RESET "\n", data->location.filename,
              data->location.line, data->location.column);
}
__attribute__((noreturn)) void
__ubsan_handle_builtin_unreachable(data_only_location_t *data) {
  debug_print(ANSI_RED "UBSAN: builtin_unreachable @ %s:%u:%u" ANSI_RESET "\n", data->location.filename,
          data->location.line, data->location.column);

  asm volatile("cli; hlt");
}
__attribute__((noreturn)) void
__ubsan_handle_missing_return(data_only_location_t *data) {
  debug_print(ANSI_RED "UBSAN: missing_return @ %s:%u:%u" ANSI_RESET "\n", data->location.filename,
          data->location.line, data->location.column);
  asm volatile("cli; hlt");
}