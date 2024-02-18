//
// Created by asxe on 2024/2/16.
//

#include "obj_map.h"
#include "class.h"
#include "../../LexicalParser/include/vm.h"
#include "obj_string.h"
#include "obj_range.h"

//创建新map对象
ObjMap *newObjMap(VM *vm) {
    ObjMap *objMap = ALLOCATE(vm, ObjMap);
    initObjHeader(vm, &objMap->objHeader, OT_MAP, vm->mapClass);
    objMap->capacity = objMap->count = 0;
    objMap->entries = NULL;
    return objMap;
}

//计算数字的哈希码
static uint32_t hashNum(double num) {
    Bits64 bits64;
    bits64.num = num;
    return bits64.bits32[0] ^ bits64.bits32[1];
}

//计算对象的哈希码
static uint32_t hashObj(ObjHeader *objHeader) {
    switch (objHeader->objType) {
        case OT_CLASS:
            return mm3HashString(((Class *) objHeader)->name->value.start, ((Class *) objHeader)->name->value.length,
                                 0);

        case OT_RANGE: {
            ObjRange *objRange = (ObjRange *) objHeader;
            return hashNum(objRange->from) ^ hashNum(objRange->to);
        }

        case OT_STRING:
            return ((ObjString *) objHeader)->hashCode;
        default:
            RUN_ERROR("the hash objects are objString, objRange and class.");
    }
    return 0;
}

//根据value的类型调用相应的哈希函数
static uint32_t hashValue(Value value) {
    switch (value.type) {
        case VT_FALSE:
            return 0;
        case VT_NULL:
            return 1;
        case VT_NUM:
            return hashNum(value.num);
        case VT_TRUE:
            return 2;
        case VT_OBJ:
            return hashObj(value.objHeader);
        default:
            RUN_ERROR("Hash types are not supported");
    }
    return 0;
}

//在entries中添加entry，如果是新的key则返回true
static bool addEntry(Entry *entries, uint32_t capacity, Value key, Value value) {
    uint32_t index = hashValue(key) % capacity;

    //通过开放探测法去找可用的slot
    while (true) {
        if (entries[index].key.type == VT_UNDEFINED) {
            entries[index].key = key;
            entries[index].value = value;
            return true; //增加新的key返回true
        } else if (valueIsEqual(entries[index].key, key)) {
            entries[index].value = value;
            return false; //未增加新的key返回false
        }
        //开放探测定址，尝试下一个slot
        index = (index + 1) % capacity;
    }
}

//使对象objMap的容量调整到newCapacity
static void resizeMap(VM *vm, ObjMap *objMap, uint32_t newCapacity) {
    Entry *newEntries = ALLOCATE_ARRAY(vm, Entry, newCapacity);
    uint32_t idx = 0;
    while (idx < newCapacity) {
        newEntries[idx].key = VT_TO_VALUE(VT_UNDEFINED);
        newEntries[idx].value = VT_TO_VALUE(VT_FALSE);
        idx++;
    }

    //把原entries中有值的部分插入到newEntries
    if (objMap->capacity > 0) {
        Entry *entryArray = objMap->entries;
        idx = 0;
        while (idx < objMap->capacity) {
            if (entryArray[idx].key.type != VT_UNDEFINED)
                addEntry(newEntries, newCapacity, entryArray[idx].key, entryArray[idx].value);
            idx++;
        }
    }

    DEALLOCATE_ARRAY(vm, objMap->entries, objMap->count);
    objMap->entries = newEntries; //更新指针为新的entries
    objMap->capacity = newCapacity; //更新容量
}




