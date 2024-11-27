#include "fwlib.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../extern/fwlib/fwlib32.h"

Fwlib::Fwlib(char* host, unsigned int port)
    : host_(host), port_(port), env_(nullptr), wrapper_(nullptr) {
  host_ = new char[strlen(host) + 1];
  strcpy(host_, host);
}

Fwlib::~Fwlib() { napi_delete_reference(env_, wrapper_); }

void Fwlib::Destructor(napi_env env, void* nativeObject,
                       void* /*finalize_hint*/) {
  reinterpret_cast<Fwlib*>(nativeObject)->~Fwlib();
  cnc_exitprocess();
}

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Fwlib::Init(napi_env env, napi_value exports) {
  napi_status status;
  napi_property_descriptor properties[] = {
      {"connected", 0, 0, GetConnected, 0, 0, napi_default, 0},
      DECLARE_NAPI_METHOD("connect", Connect),
      DECLARE_NAPI_METHOD("disconnect", Disconnect),
      DECLARE_NAPI_METHOD("rdcncid", Rdcncid),
      DECLARE_NAPI_METHOD("sysinfo", Sysinfo),
      DECLARE_NAPI_METHOD("rdaxisname", Rdaxisname),
      DECLARE_NAPI_METHOD("rdaxisdata", Rdaxisdata),
      DECLARE_NAPI_METHOD("exeprgname", Exeprgname),
      DECLARE_NAPI_METHOD("exeprgname2", Exeprgname2),
      DECLARE_NAPI_METHOD("statinfo", Statinfo),
      DECLARE_NAPI_METHOD("rddynamic2", Rddynamic2),
      DECLARE_NAPI_METHOD("rdsvmeter", Rdsvmeter),
      DECLARE_NAPI_METHOD("rdprogline", Rdprogline),
      DECLARE_NAPI_METHOD("rdprogline2", Rdprogline2),
      DECLARE_NAPI_METHOD("rdexecprog", Rdexecprog),
  };

  napi_value cons;
  status = napi_define_class(env, "Fwlib", NAPI_AUTO_LENGTH, New, nullptr, sizeof(properties),
                             properties, &cons);
  assert(status == napi_ok);
  napi_ref* constructor = new napi_ref;
  status = napi_create_reference(env, cons, 1, constructor);
  assert(status == napi_ok);
  status = napi_set_instance_data(
      env, constructor,
      [](napi_env env, void* data, void* hint) {
        napi_ref* constructor = static_cast<napi_ref*>(data);
        napi_status status = napi_delete_reference(env, *constructor);
        assert(status == napi_ok);
        delete constructor;
      },
      nullptr);
  assert(status == napi_ok);

  status = napi_set_named_property(env, exports, "Fwlib", cons);
  assert(status == napi_ok);

  short ret;
  ret = cnc_startupprocess(0, "focas.log");
  assert(ret == EW_OK);

  return exports;
}

napi_value Fwlib::Constructor(napi_env env) {
  void* instance_data = nullptr;
  napi_status status = napi_get_instance_data(env, &instance_data);
  assert(status == napi_ok);
  napi_ref* constructor = static_cast<napi_ref*>(instance_data);

  napi_value cons;
  status = napi_get_reference_value(env, *constructor, &cons);
  assert(status == napi_ok);
  return cons;
}

napi_value Fwlib::New(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value target;
  status = napi_get_new_target(env, info, &target);
  assert(status == napi_ok);
  bool is_constructor = target != nullptr;

  if (is_constructor) {
    // Invoked as constructor: `new Fwlib(...)`
    size_t argc = 2;
    napi_value args[2];
    napi_value jsthis;
    status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
    assert(status == napi_ok);

    char host[100] = "";
    unsigned int port = 8193;

    // read arg0
    napi_valuetype valuetype;
    status = napi_typeof(env, args[0], &valuetype);
    assert(status == napi_ok);

    if (valuetype != napi_undefined) {
      status =
          napi_get_value_string_utf8(env, args[0], host, sizeof(host), nullptr);
      assert(status == napi_ok);
    }

    // read arg1
    status = napi_typeof(env, args[1], &valuetype);
    assert(status == napi_ok);

    if (valuetype != napi_undefined) {
      uint32_t result;
      status = napi_get_value_uint32(env, args[1], &result);
      assert(status == napi_ok);
      port = result;
    }

    Fwlib* obj = new Fwlib(host, port);

    obj->env_ = env;
    status =
        napi_wrap(env, jsthis, reinterpret_cast<void*>(obj), Fwlib::Destructor,
                  nullptr,  // finalize_hint
                  &obj->wrapper_);
    assert(status == napi_ok);

    return jsthis;
  } else {
    // Invoked as plain function `Fwlib(...)`, turn into construct call.
    size_t argc_ = 2;
    napi_value args[2];
    status = napi_get_cb_info(env, info, &argc_, args, nullptr, nullptr);
    assert(status == napi_ok);

    const size_t argc = 2;
    napi_value argv[argc] = {args[0], args[1]};

    napi_value instance;
    status = napi_new_instance(env, Constructor(env), argc, argv, &instance);
    assert(status == napi_ok);

    return instance;
  }
}

napi_value Fwlib::GetConnected(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  napi_value num;
  status = napi_create_int32(env, obj->connected ? 1 : 0, &num);
  assert(status == napi_ok);

  return num;
}

napi_value Fwlib::Connect(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ret = cnc_allclibhndl3(obj->host_, obj->port_, 10, &obj->libh);
  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_SOCKET:
        msg = "Socket communication error";
        break;
      case EW_NODLL:
        msg = "There is no DLL file for each CNC series";
        break;
      case EW_HANDLE:
        msg = "Allocation of handle number is failed.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  obj->connected = true;

  napi_value num;
  status = napi_create_int32(env, ret, &num);
  assert(status == napi_ok);

  return num;
}

napi_value Fwlib::Disconnect(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ret = cnc_freelibhndl(obj->libh);
  obj->connected = false;

  napi_value num;
  status = napi_create_int32(env, ret, &num);
  assert(status == napi_ok);

  return num;
}

napi_value Fwlib::Sysinfo(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ODBSYS sysinfo;
  ret = cnc_sysinfo(obj->libh, &sysinfo);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg = "An unknown error occurred.";
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value val;
  status = napi_create_uint32(env, sysinfo.addinfo, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "addinfo", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.axes, 2, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "axes", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.cnc_type, 2, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "cnc_type", val);
  assert(status == napi_ok);

  status = napi_create_uint32(env, sysinfo.max_axis, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "max_axis", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.mt_type, 2, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "mt_type", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.series, 4, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "series", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, sysinfo.version, 4, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "version", val);
  assert(status == napi_ok);

  return result;
}

napi_value Fwlib::Rdcncid(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  uint32_t ids[4] = {0};
  ret = cnc_rdcncid(obj->libh, (unsigned long*)ids);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_FUNC:
        msg = "This function is not available.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_array_with_length(env, 4, &result);
  assert(status == napi_ok);
  for (int i = 0; i < 4; i++) {
    napi_value val;
    status = napi_create_uint32(env, ids[i], &val);
    assert(status == napi_ok);
    status = napi_set_element(env, result, i, val);
    assert(status == napi_ok);
  }

  return result;
}

typedef struct odbaxdt_t {
  char name[4];
  int32_t data;
  int16_t dec;
  int16_t unit;
  int16_t flag;
  int16_t reserve;
} ODBAXDT_T;

napi_value Fwlib::Rdaxisname(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  short axis_count = MAX_AXIS;
  ODBAXISNAME axis_names[MAX_AXIS];
  ret = cnc_rdaxisname(obj->libh, &axis_count, axis_names);

  if (ret != EW_OK) {
    const char* msg;
    char code[8] = "";
    switch (ret) {
      case EW_LENGTH:
        msg =
            "Data block length error: The axis number (*data_num) is 0 or "
            "less.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_array_with_length(env, axis_count, &result);
  assert(status == napi_ok);

  for (int i = 0; i < axis_count; i++) {
    napi_value elem, val;
    status = napi_create_object(env, &elem);
    char str[2] = {0};
    str[0] = axis_names[i].name;
    status = napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &val);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "name", val);
    assert(status == napi_ok);

    str[0] = axis_names[i].suff;
    status = napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &val);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "suff", val);
    assert(status == napi_ok);

    status = napi_set_element(env, result, i, elem);
    assert(status == napi_ok);
  }

  return result;
}

napi_value Fwlib::Rdaxisdata(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 2;
  napi_value args[2];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short types_length = 0;
  unsigned int type = 0;
  short types[MAX_AXIS] = {0};

  // read arg0
  napi_valuetype valuetype;
  status = napi_typeof(env, args[0], &valuetype);
  assert(status == napi_ok);

  if (valuetype != napi_undefined) {
    uint32_t arg0;
    status = napi_get_value_uint32(env, args[0], &arg0);
    assert(status == napi_ok);
    type = arg0;
  }

  // read arg1
  status = napi_typeof(env, args[1], &valuetype);
  assert(status == napi_ok);

  if (valuetype != napi_undefined) {
    bool is_array;
    status = napi_is_array(env, args[1], &is_array);
    assert(status == napi_ok);
    if (is_array) {
      uint32_t tmp0;
      status = napi_get_array_length(env, args[1], &tmp0);
      assert(status == napi_ok);
      types_length = tmp0;
      for (int i = 0; i < types_length; i++) {
        napi_value array_elem;
        status = napi_get_element(env, args[1], i, &array_elem);
        assert(status == napi_ok);

        napi_value array_num;
        status = napi_coerce_to_number(env, array_elem, &array_num);
        assert(status == napi_ok);

        int32_t array_val;
        status = napi_get_value_int32(env, array_num, &array_val);
        assert(status == napi_ok);
        types[i] = (short)array_val;
      }
    }
  }
  short ret;
  short axis_num = MAX_AXIS;
  ODBAXDT_T* axis_data = new ODBAXDT_T[MAX_AXIS * types_length];
  ODBAXDT* tmp1 = (ODBAXDT*)axis_data;
  ret = cnc_rdaxisdata(obj->libh, type, types, types_length, &axis_num, tmp1);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_LENGTH:
        msg = "Data block error: Number of axis(*len) is less or equal 0.";
        break;
      case EW_NUMBER:
        msg = "Data number error: Data class(cls) is wrong.";
        break;
      case EW_ATTRIB:
        msg =
            "Data attribute error: Kind of data(type) is wrong, or The number "
            "of kind(num) exceeds 4.";
        break;
      case EW_NOOPT:
        msg = "No option: Required option to read data is not specified.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  size_t result_arr_length = types_length * axis_num;
  status = napi_create_array_with_length(env, result_arr_length, &result);
  assert(status == napi_ok);

  for (int i = 0; i < axis_num; i++) {
    for (int j = 0; j < types_length; j++) {
      int ji = j * MAX_AXIS + i;
      napi_value elem;
      status = napi_create_object(env, &elem);
      assert(status == napi_ok);

      napi_value val;
      status = napi_create_int32(env, axis_data[ji].data, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "data", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].dec, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "dec", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].flag, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "flag", val);
      assert(status == napi_ok);

      status = napi_create_string_utf8(env, axis_data[ji].name,
                                       NAPI_AUTO_LENGTH, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "name", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].reserve, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "reserve", val);
      assert(status == napi_ok);

      status = napi_create_int32(env, axis_data[ji].unit, &val);
      assert(status == napi_ok);
      status = napi_set_named_property(env, elem, "unit", val);
      assert(status == napi_ok);

      /*
      status = napi_object_freeze(env, elem);
      assert(status == napi_ok);
      */

      status = napi_set_element(env, result, j * axis_num + i, elem);
      assert(status == napi_ok);
    }
  }
  delete[] axis_data;

  return result;
}

typedef struct odbexeprg_t {
  char name[36];
  int32_t o_num;
} ODBEXEPRG_T;

napi_value Fwlib::Exeprgname(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ODBEXEPRG_T exeprg;
  ret = cnc_exeprgname(obj->libh, (ODBEXEPRG*)&exeprg);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg = "An unknown error occurred.";
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value val;
  status = napi_create_uint32(env, exeprg.o_num, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "o_num", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, exeprg.name, NAPI_AUTO_LENGTH, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "name", val);
  assert(status == napi_ok);

  return result;
}

napi_value Fwlib::Exeprgname2(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  napi_value result;

  short ret;
  char path[256];
  ret = cnc_exeprgname2(obj->libh, path);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_FUNC:
        msg = "Not available.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  status = napi_create_string_utf8(env, path, NAPI_AUTO_LENGTH, &result);
  assert(status == napi_ok);

  return result;
}

napi_value Fwlib::Statinfo(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ODBST s;
  ret = cnc_statinfo(obj->libh, &s);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_FUNC:
        msg = "Not available.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value num;

  status = napi_create_int32(env, s.alarm, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "alarm", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.aut, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "aut", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.edit, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "edit", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.emergency, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "emergency", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.hdck, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "hdck", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.motion, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "motion", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.mstb, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "mstb", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.run, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "run", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, s.tmmode, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "tmmode", num);
  assert(status == napi_ok);

  return result;
}

typedef struct odbdy2_t {
    int16_t   dummy ;
    int16_t   axis ;
    int32_t   alarm ;
    int32_t   prgnum ;
    int32_t   prgmnum ;
    int32_t   seqnum ;
    int32_t   actf ;
    int32_t   acts ;
    union {
        struct {
            int32_t    absolute[MAX_AXIS] ;
            int32_t    machine[MAX_AXIS] ;
            int32_t    relative[MAX_AXIS] ;
            int32_t    distance[MAX_AXIS] ;
        } faxis ;
        struct {
            int32_t    absolute ;
            int32_t    machine ;
            int32_t    relative ;
            int32_t    distance ;
        } oaxis ;
    } pos ;
} ODBDY2_T ;


napi_value Fwlib::Rddynamic2(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  ODBDY2_T dyn = {0};
  ret = cnc_rddynamic2(obj->libh, ALL_AXES, sizeof(dyn), (ODBDY2 *)&dyn);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_LENGTH:
        msg =  "data block length error: Size of ODBDY structure(length) is illegal.";
        break;
      case EW_ATTRIB:
        msg = "data attribute error: The specification of axis number (axis) is improper.";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value num;

  status = napi_create_int32(env, dyn.actf, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "actf", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, dyn.acts, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "acts", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, dyn.alarm, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "alarm", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, dyn.axis, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "axis", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, dyn.dummy, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "dummy", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, dyn.prgnum, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "prgnum", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, dyn.prgmnum, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "prgmnum", num);
  assert(status == napi_ok);

  status = napi_create_int32(env, dyn.seqnum, &num);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "seqnum", num);
  assert(status == napi_ok);

  napi_value absolute;
  status = napi_create_array_with_length(env, MAX_AXIS, &absolute);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "absolute", absolute);
  assert(status == napi_ok);

  napi_value machine;
  status = napi_create_array_with_length(env, MAX_AXIS, &machine);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "machine", machine);
  assert(status == napi_ok);

  napi_value relative;
  status = napi_create_array_with_length(env, MAX_AXIS, &relative);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "relative", relative);
  assert(status == napi_ok);

  napi_value distance;
  status = napi_create_array_with_length(env, MAX_AXIS, &distance);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "distance", distance);
  assert(status == napi_ok);

  for (int i = 0; i < MAX_AXIS; i++) {
    int32_t v;
    v = dyn.pos.faxis.absolute[i];
    status = napi_create_int32(env, v, &num);
    assert(status == napi_ok);
    status = napi_set_element(env, absolute, i, num);
    assert(status == napi_ok);

    v = dyn.pos.faxis.machine[i];
    status = napi_create_int32(env, v, &num);
    assert(status == napi_ok);
    status = napi_set_element(env, machine, i, num);
    assert(status == napi_ok);

    v = dyn.pos.faxis.relative[i];
    status = napi_create_int32(env, v, &num);
    assert(status == napi_ok);
    status = napi_set_element(env, relative, i, num);
    assert(status == napi_ok);

    v = dyn.pos.faxis.distance[i];
    status = napi_create_int32(env, v, &num);
    assert(status == napi_ok);
    status = napi_set_element(env, distance, i, num);
    assert(status == napi_ok);
  }

  return result;
}

typedef struct odbsvload_t {
  struct {
      int32_t    data;
      int16_t   dec;
      int16_t   unit;
      char    name;
      char    suff1;
      char    suff2;
      char    reserve;
  } svload ;
} ODBSVLOAD_T;

napi_value Fwlib::Rdsvmeter(napi_env env, napi_callback_info info) {
  napi_status status;

  napi_value jsthis;
  status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  short ax_num = MAX_AXIS;
  ODBSVLOAD_T load[ax_num];
  ret = cnc_rdsvmeter(obj->libh, &ax_num, (ODBSVLOAD*)load);

  if (ret != EW_OK) {
    char code[8] = "";
    const char* msg;
    switch (ret) {
      case EW_LENGTH:
        msg = "Data block length error: The axis number (*data_num) is 0 or less";
        break;
      default:
        msg = "An unknown error occurred.";
    }
    snprintf(code, 7, "%d", ret);
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result, elem, num;
  status = napi_create_array_with_length(env, ax_num, &result);
  assert(status == napi_ok);

  for (int i = 0; i < ax_num; i++) {
    char s[2] = "";
    status = napi_create_object(env, &elem);
    assert(status == napi_ok);

    status = napi_create_int32(env, load[i].svload.data, &num);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "data", num);
    assert(status == napi_ok);

    status = napi_create_int32(env, load[i].svload.dec, &num);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "dec", num);
    assert(status == napi_ok);

    // always zero (%)
    status = napi_create_int32(env, load[i].svload.unit, &num);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "unit", num);
    assert(status == napi_ok);

    s[0] = load[i].svload.name;
    status = napi_create_string_utf8(env, s, strlen(s), &num);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "name", num);
    assert(status == napi_ok);

    /* garbage?
    s[0] = load[i].svload.reserve;
    status = napi_create_string_utf8(env, s, strlen(s), &num);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "reserve", num);
    assert(status == napi_ok);
    */

    s[0] = load[i].svload.suff1;
    status = napi_create_string_utf8(env, s, strlen(s), &num);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "suff", num);
    assert(status == napi_ok);

    /* "not used"
    s[0] = load[i].svload.suff2;
    status = napi_create_string_utf8(env, s, strlen(s), &num);
    assert(status == napi_ok);
    status = napi_set_named_property(env, elem, "suff2", num);
    assert(status == napi_ok);
    */

    status = napi_set_element(env, result, i, elem);
    assert(status == napi_ok);

  }
  return result;
}

napi_value Fwlib::Rdprogline(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 3;
  napi_value args[3];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok);

  if (argc < 3) {
    napi_throw_type_error(env, nullptr, "Expected 3 arguments: program number, line number, data length");
    return nullptr;
  }

  // Get the arguments from JavaScript
  int32_t prog_no;
  uint32_t line_no;
  uint32_t data_len;
  status = napi_get_value_int32(env, args[0], &prog_no);
  assert(status == napi_ok);
  status = napi_get_value_uint32(env, args[1], &line_no);
  assert(status == napi_ok);
  status = napi_get_value_uint32(env, args[2], &data_len);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  char buffer[data_len + 1];  // Allocate buffer with an extra byte for null-termination
  memset(buffer, 0, sizeof(buffer));

  unsigned long line_len = 1; // Read one line at a time
  unsigned long actual_data_len = data_len;
  ret = cnc_rdprogline(obj->libh, prog_no, line_no, buffer, &line_len, &actual_data_len);

  if (ret != EW_OK) {
    char code[8] = "";
    snprintf(code, sizeof(code), "%d", ret);
    const char* msg;
    switch (ret) {
      case EW_BUSY:
        msg = "CNC is busy or an alarm exists.";
        break;
      case EW_DATA:
        msg = "Data error: Invalid program number, line number, or line length.";
        break;
      case EW_NOOPT:
        msg = "Required option is not available.";
        break;
      case EW_MODE:
        msg = "CNC mode error.";
        break;
      case EW_PROT:
        msg = "Write protection error on CNC side.";
        break;
      case EW_REJECT:
        msg = "CNC execution denied, possibly due to MDI or background edit.";
        break;
      default:
        msg = "An unknown error occurred.";
        break;
    }
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value val;
  status = napi_create_uint32(env, line_len, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "read_lines", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, buffer, actual_data_len, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "data", val);
  assert(status == napi_ok);

  return result;
}

napi_value Fwlib::Rdprogline2(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 3;
  napi_value args[3];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok);

  if (argc < 3) {
    napi_throw_type_error(env, nullptr, "Expected 3 arguments: program number, line number, data length");
    return nullptr;
  }

  // Get the arguments from JavaScript
  int32_t prog_no;
  uint32_t line_no, data_len;
  status = napi_get_value_int32(env, args[0], &prog_no);
  assert(status == napi_ok);
  status = napi_get_value_uint32(env, args[1], &line_no);
  assert(status == napi_ok);
  status = napi_get_value_uint32(env, args[2], &data_len);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  short ret;
  char buffer[data_len + 1];  // Allocate buffer with an extra byte for null-termination
  memset(buffer, 0, sizeof(buffer));

  unsigned long line_len = 10; // Attempt to read up to 10 lines
  unsigned long actual_data_len = data_len;
  ret = cnc_rdprogline2(obj->libh, prog_no, line_no, buffer, &line_len, &actual_data_len);

  if (ret != EW_OK) {
    char code[8] = "";
    snprintf(code, sizeof(code), "%d", ret);
    const char* msg;
    switch (ret) {
      case EW_BUSY:
        msg = "CNC is busy or an alarm exists.";
        break;
      case EW_DATA:
        msg = "Data error: Invalid program number, line number, or line length.";
        break;
      case EW_NOOPT:
        msg = "Required option is not available.";
        break;
      case EW_MODE:
        msg = "CNC mode error.";
        break;
      case EW_PROT:
        msg = "Write protection error on CNC side.";
        break;
      case EW_REJECT:
        msg = "CNC execution denied, possibly due to MDI or background edit.";
        break;
      default:
        msg = "An unknown error occurred.";
        break;
    }
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value val;
  status = napi_create_uint32(env, line_len, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "read_lines", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, buffer, actual_data_len, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "data", val);
  assert(status == napi_ok);

  return result;
}

napi_value Fwlib::Rdexecprog(napi_env env, napi_callback_info info) {
  napi_status status;

  size_t argc = 1;
  napi_value args[1];
  napi_value jsthis;
  status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
  assert(status == napi_ok);

  if (argc < 1) {
    napi_throw_type_error(env, nullptr, "Expected 1 argument: data length");
    return nullptr;
  }

  // Get the arguments from JavaScript
  uint32_t data_len;
  status = napi_get_value_uint32(env, args[0], &data_len);
  assert(status == napi_ok);

  Fwlib* obj;
  status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&obj));
  assert(status == napi_ok);

  unsigned short length = data_len;
  short blknum;
  char buffer[data_len + 1];  // Allocate buffer with an extra byte for null-termination
  memset(buffer, 0, sizeof(buffer));

  short ret = cnc_rdexecprog(obj->libh, &length, &blknum, buffer);

  if (ret != EW_OK) {
    char code[8] = "";
    snprintf(code, sizeof(code), "%d", ret);
    const char* msg;
    switch (ret) {
      case EW_LENGTH:
        msg = "Data block length error: Length of the block is illegal.";
        break;
      case EW_NOOPT:
        msg = "Required option is not available.";
        break;
      default:
        msg = "An unknown error occurred.";
        break;
    }
    status = napi_throw_error(env, code, msg);
    assert(status == napi_ok);
    return nullptr;
  }

  napi_value result;
  status = napi_create_object(env, &result);
  assert(status == napi_ok);

  napi_value val;
  status = napi_create_uint32(env, length, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "read_length", val);
  assert(status == napi_ok);

  status = napi_create_int32(env, blknum, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "block_number", val);
  assert(status == napi_ok);

  status = napi_create_string_utf8(env, buffer, length, &val);
  assert(status == napi_ok);
  status = napi_set_named_property(env, result, "data", val);
  assert(status == napi_ok);

  return result;
}
