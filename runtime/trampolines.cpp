#include "trampolines.h"

#include "capi-handles.h"
#include "dict-builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"

namespace py {

// Populate the free variable and cell variable arguments.
void processFreevarsAndCellvars(Thread* thread, Frame* frame) {
  // initialize cell variables
  HandleScope scope(thread);
  Function function(&scope, frame->function());
  DCHECK(function.hasFreevarsOrCellvars(),
         "no free variables or cell variables");
  Code code(&scope, function.code());
  Runtime* runtime = thread->runtime();
  word num_locals = code.nlocals();
  word num_cellvars = code.numCellvars();
  for (word i = 0; i < code.numCellvars(); i++) {
    Cell cell(&scope, runtime->newCell());

    // Allocate a cell for a local variable if cell2arg is not preset
    if (code.cell2arg().isNoneType()) {
      frame->setLocal(num_locals + i, *cell);
      continue;
    }

    // Allocate a cell for a local variable if cell2arg is present but
    // the cell does not match any argument
    Object arg_index(&scope, Tuple::cast(code.cell2arg()).at(i));
    if (arg_index.isNoneType()) {
      frame->setLocal(num_locals + i, *cell);
      continue;
    }

    // Allocate a cell for an argument
    word local_idx = Int::cast(*arg_index).asWord();
    cell.setValue(frame->local(local_idx));
    frame->setLocal(local_idx, NoneType::object());
    frame->setLocal(num_locals + i, *cell);
  }

  // initialize free variables
  DCHECK(code.numFreevars() == 0 ||
             code.numFreevars() == Tuple::cast(function.closure()).length(),
         "Number of freevars is different than the closure.");
  for (word i = 0; i < code.numFreevars(); i++) {
    frame->setLocal(num_locals + num_cellvars + i,
                    Tuple::cast(function.closure()).at(i));
  }
}

RawObject raiseMissingArgumentsError(Thread* thread, RawFunction function,
                                     word nargs) {
  HandleScope scope(thread);
  Function function_obj(&scope, function);
  Object defaults(&scope, function_obj.defaults());
  word n_defaults = defaults.isNoneType() ? 0 : Tuple::cast(*defaults).length();
  return thread->raiseWithFmt(
      LayoutId::kTypeError,
      "'%F' takes min %w positional arguments but %w given", &function_obj,
      function_obj.argcount() - n_defaults, nargs);
}

RawObject processDefaultArguments(Thread* thread, RawFunction function_raw,
                                  Frame* frame, const word nargs) {
  word argcount = function_raw.argcount();
  word n_missing_args = argcount - nargs;
  if (n_missing_args > 0) {
    RawObject result =
        addDefaultArguments(thread, function_raw, frame, nargs, n_missing_args);
    if (result.isErrorException()) return result;
    function_raw = Function::cast(result);
    if (function_raw.hasSimpleCall()) {
      return function_raw;
    }
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, function_raw);
  Object varargs_param(&scope, runtime->emptyTuple());
  if (n_missing_args < 0) {
    // We have too many arguments.
    if (!function.hasVarargs()) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "'%F' takes max %w positional arguments but %w given", &function,
          argcount, nargs);
    }
    // Put extra positional args into the varargs tuple.
    word len = -n_missing_args;
    Tuple tuple(&scope, runtime->newTuple(len));
    for (word i = (len - 1); i >= 0; i--) {
      tuple.atPut(i, frame->popValue());
    }
    varargs_param = *tuple;
  }

  // If there are any keyword-only args, there must be defaults for them
  // because we arrived here via CALL_FUNCTION (and thus, no keywords were
  // supplied at the call site).
  Code code(&scope, function.code());
  word kwonlyargcount = code.kwonlyargcount();
  if (kwonlyargcount > 0) {
    if (function.kwDefaults().isNoneType()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "missing keyword-only argument");
    }
    Dict kw_defaults(&scope, function.kwDefaults());
    if (!kw_defaults.isNoneType()) {
      Tuple formal_names(&scope, code.varnames());
      word first_kw = argcount;
      Object name(&scope, NoneType::object());
      for (word i = 0; i < kwonlyargcount; i++) {
        name = formal_names.at(first_kw + i);
        RawObject value = dictAtByStr(thread, kw_defaults, name);
        if (value.isError()) {
          return thread->raiseWithFmt(LayoutId::kTypeError,
                                      "missing keyword-only argument");
        }
        frame->pushValue(value);
      }
    }
  }

  if (function.hasVarargs()) {
    frame->pushValue(*varargs_param);
  }
  if (function.hasVarkeyargs()) {
    // VARKEYARGS - because we arrived via CALL_FUNCTION, no keyword arguments
    // provided.  Just add an empty dict.
    frame->pushValue(runtime->newDict());
  }
  return *function;
}

// Verify correct number and order of arguments.  If order is wrong, try to
// fix it.  If argument is missing (denoted by Error::object()), try to supply
// it with a default.  This routine expects the number of args on the stack
// and number of names in the actual_names tuple to match.  Caller must pad
// prior to calling to ensure this.
// Return None::object() if successful, error object if not.
static RawObject checkArgs(Thread* thread, const Function& function,
                           RawObject* kw_arg_base, const Tuple& actual_names,
                           const Tuple& formal_names, word start) {
  word posonlyargcount = RawCode::cast(function.code()).posonlyargcount();
  word num_actuals = actual_names.length();
  // Helper function to swap actual arguments and names
  auto swap = [&kw_arg_base, &actual_names](word arg_pos1,
                                            word arg_pos2) -> void {
    RawObject tmp = *(kw_arg_base - arg_pos1);
    *(kw_arg_base - arg_pos1) = *(kw_arg_base - arg_pos2);
    *(kw_arg_base - arg_pos2) = tmp;
    tmp = actual_names.at(arg_pos1);
    actual_names.atPut(arg_pos1, actual_names.at(arg_pos2));
    actual_names.atPut(arg_pos2, tmp);
  };
  // Helper function to retrieve argument
  auto arg_at = [&kw_arg_base](word idx) -> RawObject& {
    return *(kw_arg_base - idx);
  };
  HandleScope scope(thread);
  Object formal_name(&scope, NoneType::object());
  for (word arg_pos = 0; arg_pos < num_actuals; arg_pos++) {
    word formal_pos = arg_pos + start;
    formal_name = formal_names.at(formal_pos);
    RawObject result =
        Runtime::objectEquals(thread, actual_names.at(arg_pos), *formal_name);
    if (result.isErrorException()) return result;
    if (result == Bool::trueObj()) {
      if (formal_pos >= posonlyargcount) {
        // We're good here: actual & formal arg names match.  Check the next
        // one.
        continue;
      }
      // A matching keyword arg but for a positional-only parameter.
      return Thread::current()->raiseWithFmt(
          LayoutId::kTypeError,
          "keyword argument specified for positional-only argument '%S'",
          &formal_name);
    }
    // Mismatch.  Try to fix it.  Note: args grow down.
    bool swapped = false;
    // Look for expected Formal name in Actuals tuple.
    for (word i = arg_pos + 1; i < num_actuals; i++) {
      result = Runtime::objectEquals(thread, actual_names.at(i), *formal_name);
      if (result.isErrorException()) return result;
      if (result == Bool::trueObj()) {
        // Found it.  Swap both the stack and the actual_names tuple.
        swap(arg_pos, i);
        swapped = true;
        break;
      }
    }
    if (swapped) {
      // We managed to fix it.  Check the next one.
      continue;
    }
    // Can't find an Actual for this Formal.
    // If we have a real actual in current slot, move it somewhere safe.
    if (!arg_at(arg_pos).isError()) {
      for (word i = arg_pos + 1; i < num_actuals; i++) {
        if (arg_at(i).isError()) {
          // Found an uninitialized slot.  Use it to save current actual.
          swap(arg_pos, i);
          break;
        }
      }
      // If we were unable to find a slot to swap into, TypeError
      if (!arg_at(arg_pos).isError()) {
        Object param_name(&scope, swapped ? formal_names.at(arg_pos)
                                          : actual_names.at(arg_pos));
        return thread->raiseWithFmt(
            LayoutId::kTypeError,
            "%F() got an unexpected keyword argument '%S'", &function,
            &param_name);
      }
    }
    // Now, can we fill that slot with a default argument?
    word absolute_pos = arg_pos + start;
    word argcount = function.argcount();
    if (absolute_pos < argcount) {
      word defaults_size = function.hasDefaults()
                               ? Tuple::cast(function.defaults()).length()
                               : 0;
      word defaults_start = argcount - defaults_size;
      if (absolute_pos >= (defaults_start)) {
        // Set the default value
        Tuple default_args(&scope, function.defaults());
        *(kw_arg_base - arg_pos) =
            default_args.at(absolute_pos - defaults_start);
        continue;  // Got it, move on to the next
      }
    } else if (!function.kwDefaults().isNoneType()) {
      // How about a kwonly default?
      Dict kw_defaults(&scope, function.kwDefaults());
      Str name(&scope, formal_names.at(arg_pos + start));
      RawObject val = dictAtByStr(thread, kw_defaults, name);
      if (!val.isError()) {
        *(kw_arg_base - arg_pos) = val;
        continue;  // Got it, move on to the next
      }
    }
    return thread->raiseWithFmt(LayoutId::kTypeError, "missing argument");
  }
  return NoneType::object();
}

static word findName(Thread* thread, word posonlyargcount, const Object& name,
                     const Tuple& names) {
  word len = names.length();
  for (word i = posonlyargcount; i < len; i++) {
    RawObject result = Runtime::objectEquals(thread, *name, names.at(i));
    if (result.isErrorException()) return -1;
    if (result == Bool::trueObj()) {
      return i;
    }
  }
  return len;
}

// Converts the outgoing arguments of a keyword call into positional arguments
// and processes default arguments, rearranging everything into a form expected
// by the callee.
RawObject prepareKeywordCall(Thread* thread, RawFunction function_raw,
                             Frame* frame, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, function_raw);
  // Destructively pop the tuple of kwarg names
  Tuple keywords(&scope, frame->topValue());
  frame->popValue();
  Code code(&scope, function.code());
  word expected_args = function.argcount() + code.kwonlyargcount();
  word num_keyword_args = keywords.length();
  word num_positional_args = nargs - num_keyword_args;
  Tuple varnames(&scope, code.varnames());
  Object tmp_varargs(&scope, NoneType::object());
  Object tmp_dict(&scope, NoneType::object());

  // We expect use of keyword argument calls to be uncommon, but when used
  // we anticipate mostly use of simple forms.  General scheme here is to
  // normalize the odd forms into standard form and then handle them all
  // in the same place.
  if (function.hasVarargsOrVarkeyargs()) {
    Runtime* runtime = thread->runtime();
    if (function.hasVarargs()) {
      // If we have more positional than expected, add the remainder to a tuple,
      // remove from the stack and close up the hole.
      word excess =
          Utils::maximum<word>(0, num_positional_args - function.argcount());
      Tuple varargs(&scope, runtime->newTuple(excess));
      if (excess > 0) {
        // Point to the leftmost excess argument
        RawObject* p = (frame->valueStackTop() + num_keyword_args + excess) - 1;
        // Copy the excess to the * tuple
        for (word i = 0; i < excess; i++) {
          varargs.atPut(i, *(p - i));
        }
        // Fill in the hole
        for (word i = 0; i < num_keyword_args; i++) {
          *p = *(p - excess);
          p--;
        }
        // Adjust the counts
        frame->dropValues(excess);
        nargs -= excess;
        num_positional_args -= excess;
      }
      tmp_varargs = *varargs;
    }
    if (function.hasVarkeyargs()) {
      // Too many positional args passed?
      if (num_positional_args > function.argcount()) {
        return thread->raiseWithFmt(LayoutId::kTypeError,
                                    "Too many positional arguments");
      }
      // If we have keyword arguments that don't appear in the formal parameter
      // list, add them to a keyword dict.
      Dict dict(&scope, runtime->newDict());
      List saved_keyword_list(&scope, runtime->newList());
      List saved_values(&scope, runtime->newList());
      DCHECK(varnames.length() >= expected_args,
             "varnames must be greater than or equal to positional args");
      RawObject* p = frame->valueStackTop() + (num_keyword_args - 1);
      word posonlyargcount = code.posonlyargcount();
      for (word i = 0; i < num_keyword_args; i++) {
        Object key(&scope, keywords.at(i));
        Object value(&scope, *(p - i));
        word result = findName(thread, posonlyargcount, key, varnames);
        if (result < 0) return Error::exception();
        if (result < expected_args) {
          // Got a match, stash pair for future restoration on the stack
          runtime->listAdd(thread, saved_keyword_list, key);
          runtime->listAdd(thread, saved_values, value);
        } else {
          // New, add it and associated value to the varkeyargs dict
          Object hash_obj(&scope, Interpreter::hash(thread, key));
          if (hash_obj.isErrorException()) return *hash_obj;
          word hash = SmallInt::cast(*hash_obj).value();
          Object dict_result(&scope, dictAtPut(thread, dict, key, hash, value));
          if (dict_result.isErrorException()) return *dict_result;
          nargs--;
        }
      }
      // Now, restore the stashed values to the stack and build a new
      // keywords name list.
      frame->dropValues(num_keyword_args);  // Pop all of the old keyword values
      num_keyword_args = saved_keyword_list.numItems();
      // Replace the old keywords list with a new one.
      keywords = runtime->newTuple(num_keyword_args);
      for (word i = 0; i < num_keyword_args; i++) {
        frame->pushValue(saved_values.at(i));
        keywords.atPut(i, saved_keyword_list.at(i));
      }
      tmp_dict = *dict;
    }
  }
  // At this point, all vararg forms have been normalized
  RawObject* kw_arg_base = (frame->valueStackTop() + num_keyword_args) -
                           1;  // pointer to first non-positional arg
  if (UNLIKELY(nargs > expected_args)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "Too many arguments");
  }
  if (UNLIKELY(nargs < expected_args)) {
    // Too few args passed.  Can we supply default args to make it work?
    // First, normalize & pad keywords and stack arguments
    word name_tuple_size = expected_args - num_positional_args;
    Tuple padded_keywords(&scope, thread->runtime()->newTuple(name_tuple_size));
    for (word i = 0; i < num_keyword_args; i++) {
      padded_keywords.atPut(i, keywords.at(i));
    }
    // Fill in missing spots w/ Error code
    for (word i = num_keyword_args; i < name_tuple_size; i++) {
      frame->pushValue(Error::error());
      padded_keywords.atPut(i, Error::error());
    }
    keywords = *padded_keywords;
  }
  // Now we've got the right number.  Do they match up?
  RawObject res = checkArgs(thread, function, kw_arg_base, keywords, varnames,
                            num_positional_args);
  if (res.isError()) {
    return res;  // TypeError created by checkArgs.
  }
  CHECK(res.isNoneType(), "checkArgs should return an Error or None");
  // If we're a vararg form, need to push the tuple/dict.
  if (function.hasVarargs()) {
    frame->pushValue(*tmp_varargs);
  }
  if (function.hasVarkeyargs()) {
    frame->pushValue(*tmp_dict);
  }
  return *function;
}

// Converts explode arguments into positional arguments.
//
// Returns the new number of positional arguments as a SmallInt, or Error if an
// exception was raised (most likely due to a non-string keyword name).
static RawObject processExplodeArguments(Thread* thread, Frame* frame,
                                         word flags) {
  HandleScope scope(thread);
  Object kw_mapping(&scope, NoneType::object());
  if (flags & CallFunctionExFlag::VAR_KEYWORDS) {
    kw_mapping = frame->topValue();
    frame->popValue();
  }
  Tuple positional_args(&scope, frame->popValue());
  word nargs = positional_args.length();
  for (word i = 0; i < nargs; i++) {
    frame->pushValue(positional_args.at(i));
  }
  Runtime* runtime = thread->runtime();
  if (flags & CallFunctionExFlag::VAR_KEYWORDS) {
    if (!kw_mapping.isDict()) {
      DCHECK(runtime->isMapping(thread, kw_mapping),
             "kw_mapping must have __getitem__");
      Dict dict(&scope, runtime->newDict());
      Object result(&scope, dictMergeIgnore(thread, dict, kw_mapping));
      if (result.isError()) {
        if (thread->pendingExceptionType() ==
            runtime->typeAt(LayoutId::kAttributeError)) {
          thread->clearPendingException();
          return thread->raiseWithFmt(LayoutId::kTypeError,
                                      "argument must be a mapping, not %T\n",
                                      &kw_mapping);
        }
        return *result;
      }
      kw_mapping = *dict;
    }
    Dict dict(&scope, *kw_mapping);
    word len = dict.numItems();
    if (len == 0) {
      frame->pushValue(runtime->emptyTuple());
      return SmallInt::fromWord(nargs);
    }
    MutableTuple keys(&scope, runtime->newMutableTuple(len));
    Object key(&scope, NoneType::object());
    Object value(&scope, NoneType::object());
    for (word i = 0, j = 0; dictNextItem(dict, &i, &key, &value); j++) {
      if (!runtime->isInstanceOfStr(*key)) {
        return thread->raiseWithFmt(LayoutId::kTypeError,
                                    "keywords must be strings");
      }
      keys.atPut(j, *key);
      frame->pushValue(*value);
    }
    nargs += len;
    frame->pushValue(keys.becomeImmutable());
  }
  return SmallInt::fromWord(nargs);
}

// Takes the outgoing arguments of an explode argument call and rearranges them
// into the form expected by the callee.
RawObject prepareExplodeCall(Thread* thread, RawFunction function_raw,
                             Frame* frame, word flags) {
  HandleScope scope(thread);
  Function function(&scope, function_raw);

  RawObject arg_obj = processExplodeArguments(thread, frame, flags);
  if (arg_obj.isError()) return arg_obj;
  word new_argc = SmallInt::cast(arg_obj).value();

  if (flags & CallFunctionExFlag::VAR_KEYWORDS) {
    RawObject result = prepareKeywordCall(thread, *function, frame, new_argc);
    if (result.isError()) {
      return result;
    }
  } else {
    // Are we one of the less common cases?
    if (new_argc != function.argcount() || !(function.hasSimpleCall())) {
      RawObject result =
          processDefaultArguments(thread, *function, frame, new_argc);
      if (result.isError()) {
        return result;
      }
    }
  }
  return *function;
}

static RawObject createGeneratorObject(Runtime* runtime,
                                       const Function& function) {
  if (function.isGenerator()) return runtime->newGenerator();
  if (function.isCoroutine()) return runtime->newCoroutine();
  DCHECK(function.isAsyncGenerator(), "unexpected type");
  return runtime->newAsyncGenerator();
}

static RawObject createGenerator(Thread* thread, const Function& function,
                                 const Str& qualname) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  GeneratorFrame generator_frame(&scope, runtime->newGeneratorFrame(function));
  thread->popFrameToGeneratorFrame(generator_frame);
  GeneratorBase gen_base(&scope, createGeneratorObject(runtime, function));
  gen_base.setGeneratorFrame(*generator_frame);
  gen_base.setExceptionState(runtime->newExceptionState());
  gen_base.setQualname(*qualname);
  return *gen_base;
}

RawObject generatorTrampoline(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, frame->peek(nargs));
  RawObject error = preparePositionalCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    return Error::exception();
  }
  Str qualname(&scope, function.qualname());
  return createGenerator(thread, function, qualname);
}

RawObject generatorTrampolineKw(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip over the keyword dictionary to read the function object.
  Function function(&scope, frame->peek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    return Error::exception();
  }
  Str qualname(&scope, function.qualname());
  return createGenerator(thread, function, qualname);
}

RawObject generatorTrampolineEx(Thread* thread, Frame* frame, word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, frame->peek((flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  RawObject error = prepareExplodeCall(thread, *function, frame, flags);
  if (error.isError()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    return Error::exception();
  }
  Str qualname(&scope, function.qualname());
  return createGenerator(thread, function, qualname);
}

RawObject generatorClosureTrampoline(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, frame->peek(nargs));
  RawObject error = preparePositionalCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  Str qualname(&scope, function.qualname());
  return createGenerator(thread, function, qualname);
}

RawObject generatorClosureTrampolineKw(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, frame->peek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  Str qualname(&scope, function.qualname());
  return createGenerator(thread, function, qualname);
}

RawObject generatorClosureTrampolineEx(Thread* thread, Frame* frame,
                                       word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, frame->peek((flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  RawObject error = prepareExplodeCall(thread, *function, frame, flags);
  if (error.isError()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  Str qualname(&scope, function.qualname());
  return createGenerator(thread, function, qualname);
}

RawObject interpreterTrampoline(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, frame->peek(nargs));
  RawObject error = preparePositionalCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    return Error::exception();
  }
  return Interpreter::execute(thread);
}

RawObject interpreterTrampolineKw(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, frame->peek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    return Error::exception();
  }
  return Interpreter::execute(thread);
}

RawObject interpreterTrampolineEx(Thread* thread, Frame* frame, word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, frame->peek((flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  RawObject error = prepareExplodeCall(thread, *function, frame, flags);
  if (error.isError()) {
    return error;
  }
  if (UNLIKELY(thread->pushCallFrame(*function) == nullptr)) {
    return Error::exception();
  }
  return Interpreter::execute(thread);
}

RawObject interpreterClosureTrampoline(Thread* thread, Frame* frame,
                                       word nargs) {
  HandleScope scope(thread);
  Function function(&scope, frame->peek(nargs));
  RawObject error = preparePositionalCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return Interpreter::execute(thread);
}

RawObject interpreterClosureTrampolineKw(Thread* thread, Frame* frame,
                                         word nargs) {
  HandleScope scope(thread);
  // The argument does not include the hidden keyword dictionary argument.  Add
  // one to skip the keyword dictionary to get to the function object.
  Function function(&scope, frame->peek(nargs + 1));
  RawObject error = prepareKeywordCall(thread, *function, frame, nargs);
  if (error.isError()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return Interpreter::execute(thread);
}

RawObject interpreterClosureTrampolineEx(Thread* thread, Frame* frame,
                                         word flags) {
  HandleScope scope(thread);
  // The argument is either zero when there is one argument and one when there
  // are two arguments.  Skip over these arguments to read the function object.
  Function function(
      &scope, frame->peek((flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1));
  RawObject error = prepareExplodeCall(thread, *function, frame, flags);
  if (error.isError()) {
    return error;
  }
  Frame* callee_frame = thread->pushCallFrame(*function);
  if (UNLIKELY(callee_frame == nullptr)) {
    return Error::exception();
  }
  processFreevarsAndCellvars(thread, callee_frame);
  return Interpreter::execute(thread);
}

// method no args

static RawObject callMethNoArgs(Thread* thread, const Function& function,
                                const Object& self) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, nullptr);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return *result;
}

RawObject methodTrampolineNoArgs(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes no arguments");
  }
  HandleScope scope(thread);
  Function function(&scope, frame->peek(1));
  Object self(&scope, frame->peek(0));
  return callMethNoArgs(thread, function, self);
}

RawObject methodTrampolineNoArgsKw(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Tuple kwargs(&scope, frame->peek(0));
  if (kwargs.length() != 0) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes no keyword arguments");
  }
  if (nargs != 0) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes no arguments");
  }
  Function function(&scope, frame->peek(1));
  Object self(&scope, frame->peek(0));
  return callMethNoArgs(thread, function, self);
}

RawObject methodTrampolineNoArgsEx(Thread* thread, Frame* frame, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Tuple varargs(&scope, frame->peek(has_varkeywords));
  if (has_varkeywords) {
    Object kw_args(&scope, frame->topValue());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "function takes no keyword arguments");
    }
  }
  if (varargs.length() != 1) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes no arguments");
  }
  Function function(&scope, frame->peek(has_varkeywords + 1));
  Object self(&scope, varargs.at(0));
  return callMethNoArgs(thread, function, self);
}

// method one arg

static RawObject callMethOneArg(Thread* thread, const Function& function,
                                const Object& self, const Object& arg) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* arg_obj = ApiHandle::newReference(thread, *arg);
  PyObject* pyresult = (*method)(self_obj, arg_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(arg_obj)->decref();
  return *result;
}

RawObject methodTrampolineOneArg(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes exactly one argument");
  }
  HandleScope scope(thread);
  Function function(&scope, frame->peek(2));
  Object self(&scope, frame->peek(1));
  Object arg(&scope, frame->peek(0));
  return callMethOneArg(thread, function, self, arg);
}

RawObject methodTrampolineOneArgKw(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Tuple kwargs(&scope, frame->peek(0));
  if (kwargs.length() != 0) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes no keyword arguments");
  }
  if (nargs != 2) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes exactly one arguments");
  }
  Function function(&scope, frame->peek(3));
  Object self(&scope, frame->peek(1));
  Object arg(&scope, frame->peek(2));
  return callMethOneArg(thread, function, self, arg);
}

RawObject methodTrampolineOneArgEx(Thread* thread, Frame* frame, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  if (has_varkeywords) {
    Object kw_args(&scope, frame->topValue());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "function takes no keyword arguments");
    }
  }
  Tuple varargs(&scope, frame->peek(has_varkeywords));
  if (varargs.length() != 2) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes exactly one argument");
  }
  Object self(&scope, varargs.at(0));
  Object arg(&scope, varargs.at(1));
  Function function(&scope, frame->peek(has_varkeywords + 1));
  return callMethOneArg(thread, function, self, arg);
}

// callMethVarArgs

static RawObject callMethVarArgs(Thread* thread, const Function& function,
                                 const Object& self, const Object& varargs) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  binaryfunc method = bit_cast<binaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* varargs_obj = ApiHandle::newReference(thread, *varargs);
  PyObject* pyresult = (*method)(self_obj, varargs_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(varargs_obj)->decref();
  return *result;
}

RawObject methodTrampolineVarArgs(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes at least one arguments");
  }
  HandleScope scope(thread);
  Function function(&scope, frame->peek(nargs));
  Object self(&scope, frame->peek(nargs - 1));
  Tuple varargs(&scope, thread->runtime()->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(nargs - i - 2, frame->peek(i));
  }
  return callMethVarArgs(thread, function, self, varargs);
}

RawObject methodTrampolineVarArgsKw(Thread* thread, Frame* frame, word nargs) {
  DCHECK(nargs > 1, "nargs must be greater than 1");
  HandleScope scope(thread);
  Tuple kwargs(&scope, frame->peek(0));
  if (kwargs.length() != 0) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "function takes no keyword arguments");
  }
  Function function(&scope, frame->peek(nargs + 1));
  Object self(&scope, frame->peek(nargs - 1));
  Tuple varargs(&scope, thread->runtime()->newTuple(nargs));
  for (word i = 1; i < nargs; i++) {
    varargs.atPut(nargs - i - 1, frame->peek(i + 1));
  }
  return callMethVarArgs(thread, function, self, varargs);
}

RawObject methodTrampolineVarArgsEx(Thread* thread, Frame* frame, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  if (has_varkeywords) {
    Object kw_args(&scope, frame->topValue());
    if (!kw_args.isDict()) UNIMPLEMENTED("mapping kwargs");
    if (Dict::cast(*kw_args).numItems() != 0) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "function takes no keyword arguments");
    }
  }
  Function function(&scope, frame->peek(has_varkeywords + 1));
  Tuple varargs(&scope, frame->peek(has_varkeywords));
  Object self(&scope, varargs.at(0));
  Object args(&scope, thread->runtime()->tupleSubseq(thread, varargs, 1,
                                                     varargs.length() - 1));
  return callMethVarArgs(thread, function, self, args);
}

// callMethKeywordArgs

static RawObject callMethKeywords(Thread* thread, const Function& function,
                                  const Object& self, const Object& args,
                                  const Object& kwargs) {
  HandleScope scope(thread);
  Int address(&scope, function.code());
  ternaryfunc method = bit_cast<ternaryfunc>(address.asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* args_obj = ApiHandle::newReference(thread, *args);
  PyObject* kwargs_obj = nullptr;
  if (*kwargs != NoneType::object()) {
    kwargs_obj = ApiHandle::newReference(thread, *kwargs);
  }
  PyObject* pyresult = (*method)(self_obj, args_obj, kwargs_obj);
  Object result(&scope, ApiHandle::checkFunctionResult(thread, pyresult));
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  ApiHandle::fromPyObject(args_obj)->decref();
  if (kwargs_obj != nullptr) {
    ApiHandle::fromPyObject(kwargs_obj)->decref();
  }
  return *result;
}

RawObject methodTrampolineKeywords(Thread* thread, Frame* frame, word nargs) {
  DCHECK(nargs > 0, "nargs must be greater than 0");
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Function function(&scope, frame->peek(nargs));
  Object self(&scope, frame->peek(nargs - 1));
  Tuple varargs(&scope, runtime->newTuple(nargs - 1));
  for (word i = 0; i < nargs - 1; i++) {
    varargs.atPut(nargs - i - 2, frame->peek(i));
  }
  Object keywords(&scope, NoneType::object());
  return callMethKeywords(thread, function, self, varargs, keywords);
}

RawObject methodTrampolineKeywordsKw(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple kw_names(&scope, frame->peek(0));
  Object kwargs(&scope, NoneType::object());
  word num_keywords = kw_names.length();
  if (num_keywords != 0) {
    Dict dict(&scope, runtime->newDict());
    for (word i = 0; i < num_keywords; i++) {
      Str name(&scope, kw_names.at(i));
      Object value(&scope, frame->peek(num_keywords - i));
      dictAtPutByStr(thread, dict, name, value);
    }
    kwargs = *dict;
  }
  word num_positional = nargs - num_keywords - 1;
  Tuple args(&scope, runtime->newTuple(num_positional));
  for (word i = 0; i < num_positional; i++) {
    args.atPut(i, frame->peek(nargs - i - 1));
  }
  Function function(&scope, frame->peek(nargs + 1));
  Object self(&scope, frame->peek(nargs));
  return callMethKeywords(thread, function, self, args, kwargs);
}

RawObject methodTrampolineKeywordsEx(Thread* thread, Frame* frame, word flags) {
  HandleScope scope(thread);
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  Tuple varargs(&scope, frame->peek(has_varkeywords));
  Object kwargs(&scope, NoneType::object());
  if (has_varkeywords) {
    kwargs = frame->topValue();
    if (!kwargs.isDict()) UNIMPLEMENTED("mapping kwargs");
  }
  Function function(&scope, frame->peek(has_varkeywords + 1));
  Object self(&scope, varargs.at(0));
  Object args(&scope, thread->runtime()->tupleSubseq(thread, varargs, 1,
                                                     varargs.length() - 1));
  return callMethKeywords(thread, function, self, args, kwargs);
}

// callMethFastCall

static RawObject callMethFastCallWithKwargs(Thread* thread,
                                            const Function& function,
                                            const Object& self, PyObject** args,
                                            const word num_args,
                                            const Object& kwnames) {
  _PyCFunctionFast method =
      bit_cast<_PyCFunctionFast>(Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  ApiHandle* kwnames_obj = ApiHandle::newReference(thread, *kwnames);
  PyObject* pyresult = (*method)(self_obj, args, num_args, kwnames_obj);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  kwnames_obj->decref();
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

static RawObject callMethFastCall(Thread* thread, const Function& function,
                                  const Object& self, PyObject** args,
                                  const word num_args) {
  _PyCFunctionFast method =
      bit_cast<_PyCFunctionFast>(Int::cast(function.code()).asCPtr());
  PyObject* self_obj =
      self.isUnbound() ? nullptr : ApiHandle::newReference(thread, *self);
  PyObject* pyresult = (*method)(self_obj, args, num_args, /*kwnames=*/nullptr);
  RawObject result = ApiHandle::checkFunctionResult(thread, pyresult);
  if (self_obj != nullptr) {
    ApiHandle::fromPyObject(self_obj)->decref();
  }
  return result;
}

RawObject methodTrampolineFastCall(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, frame->peek(nargs));
  Object self(&scope, frame->peek(nargs - 1));

  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[nargs - 1]);
  for (word i = 0; i < nargs - 1; i++) {
    fastcall_args[nargs - i - 2] =
        ApiHandle::newReference(thread, frame->peek(i));
  }
  word num_positional = nargs - 1;
  Object result(&scope, callMethFastCall(thread, function, self,
                                         fastcall_args.get(), num_positional));
  for (word i = 0; i < nargs - 1; i++) {
    ApiHandle::fromPyObject(fastcall_args[nargs - i - 2])->decref();
  }
  return *result;
}

RawObject methodTrampolineFastCallKw(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Function function(&scope, frame->peek(nargs + 1));
  Object self(&scope, frame->peek(nargs));

  std::unique_ptr<PyObject*[]> fastcall_args(new PyObject*[nargs - 1]);
  for (word i = 0; i < nargs - 1; i++) {
    fastcall_args[i] =
        ApiHandle::newReference(thread, frame->peek(nargs - i - 1));
  }
  Tuple kwnames(&scope, frame->peek(0));
  word num_positional = nargs - kwnames.length() - 1;
  Object result(&scope, callMethFastCallWithKwargs(thread, function, self,
                                                   fastcall_args.get(),
                                                   num_positional, kwnames));
  for (word i = 0; i < nargs - 1; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  return *result;
}

RawObject methodTrampolineFastCallEx(Thread* thread, Frame* frame, word flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  bool has_varkeywords = flags & CallFunctionExFlag::VAR_KEYWORDS;
  word num_keywords = 0;

  // Get the keyword arguments
  Tuple kwnames_tuple(&scope, runtime->emptyTuple());
  if (has_varkeywords) {
    Dict dict(&scope, frame->topValue());
    List dict_keys(&scope, dictKeys(thread, dict));
    kwnames_tuple = runtime->newTuple(dict_keys.numItems());
    num_keywords = kwnames_tuple.length();
    for (word j = 0; j < num_keywords; j++) {
      kwnames_tuple.atPut(j, dict_keys.at(j));
    }
  }

  Tuple varargs(&scope, frame->peek(has_varkeywords));
  word num_positional = varargs.length() - 1;
  std::unique_ptr<PyObject*[]> fastcall_args(
      new PyObject*[num_positional + num_keywords]);

  // Set the positional arguments
  for (word i = 0; i < num_positional; i++) {
    fastcall_args[i] = ApiHandle::newReference(thread, varargs.at(i + 1));
  }

  // Set the keyword arguments
  if (has_varkeywords) {
    Dict dict(&scope, frame->topValue());
    for (word i = num_positional; i < (num_positional + num_keywords); i++) {
      Str key(&scope, kwnames_tuple.at(i - num_positional));
      fastcall_args[i] =
          ApiHandle::newReference(thread, dictAtByStr(thread, dict, key));
    }
  }

  Function function(&scope, frame->peek(has_varkeywords + 1));
  Object self(&scope, varargs.at(0));

  Object result(&scope, NoneType::object());
  if (!has_varkeywords) {
    result = callMethFastCall(thread, function, self, fastcall_args.get(),
                              num_positional);
  } else {
    result =
        callMethFastCallWithKwargs(thread, function, self, fastcall_args.get(),
                                   num_positional, kwnames_tuple);
  }
  for (word i = 0; i < num_positional + num_keywords; i++) {
    ApiHandle::fromPyObject(fastcall_args[i])->decref();
  }
  return *result;
}

RawObject unimplementedTrampoline(Thread*, Frame*, word) {
  UNIMPLEMENTED("Trampoline");
}

static inline RawObject builtinTrampolineImpl(Thread* thread,
                                              Frame* caller_frame, word arg,
                                              word function_idx,
                                              PrepareCallFunc prepare_call) {
  // Warning: This code is using `RawXXX` variables for performance! This is
  // despite the fact that we call functions that do potentially perform memory
  // allocations. This is legal here because we always rely on the functions
  // returning an up-to-date address and we make sure to never access any value
  // produce before a call after that call. Be careful not to break this
  // invariant if you change the code!

  RawObject prepare_result =
      prepare_call(thread, Function::cast(caller_frame->peek(function_idx)),
                   caller_frame, arg);
  if (prepare_result.isError()) return prepare_result;
  RawFunction function = Function::cast(prepare_result);

  RawObject result = NoneType::object();
  {
    DCHECK(!function.code().isNoneType(),
           "builtin functions should have annotated code objects");
    RawCode code = Code::cast(function.code());
    DCHECK(code.code().isSmallInt(),
           "builtin functions should contain entrypoint in code.code");
    void* entry = SmallInt::cast(code.code()).asCPtr();

    word nargs = function.totalArgs();
    Frame* callee_frame = thread->pushNativeFrame(nargs);
    if (UNLIKELY(callee_frame == nullptr)) {
      return Error::exception();
    }
    result = bit_cast<Function::Entry>(entry)(thread, callee_frame, nargs);
    // End scope so people do not accidentally use raw variables after the call
    // which could have triggered a GC.
  }
  DCHECK(thread->isErrorValueOk(result), "error/exception mismatch");
  thread->popFrame();
  return result;
}

RawObject builtinTrampoline(Thread* thread, Frame* frame, word nargs) {
  return builtinTrampolineImpl(thread, frame, nargs, /*function_idx=*/nargs,
                               preparePositionalCall);
}

RawObject builtinTrampolineKw(Thread* thread, Frame* frame, word nargs) {
  return builtinTrampolineImpl(thread, frame, nargs, /*function_idx=*/nargs + 1,
                               prepareKeywordCall);
}

RawObject builtinTrampolineEx(Thread* thread, Frame* frame, word flags) {
  return builtinTrampolineImpl(
      thread, frame, flags,
      /*function_idx=*/(flags & CallFunctionExFlag::VAR_KEYWORDS) ? 2 : 1,
      prepareExplodeCall);
}

}  // namespace py
