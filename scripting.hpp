#ifndef SCRIPTING_HPP
#define SCRIPTING_HPP

#include <v8.h>
#include <string>

namespace ADWIF
{
  class Scripting
  {
  public:
    static void createContext(v8::Persistent<v8::Context> & out, v8::Isolate * isolate = v8::Isolate::GetCurrent());
    static bool executeScript(const std::string & source, std::string & exception, v8::Handle<v8::Value> & result /*, v8::Isolate * isolate = v8::Isolate::GetCurrent() */);

    template<class T>
    static v8::Handle<T> PersistentToHandle(v8::Persistent<T> & persistent, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      v8::Handle<T> templ = v8::Local<T>::New(isolate, persistent);
      return handle_scope.Close(templ);
    }

    template<class T>
    static void HandleToPersistent(v8::Handle<T> handle, v8::Persistent<T> & persistent, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      persistent.Reset(isolate, handle);
    }

    template<class T>
    static v8::Handle<v8::Object> wrap(T * object, v8::Handle<v8::ObjectTemplate> templ, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      v8::Handle<v8::Object> result = templ->NewInstance();
      v8::Handle<v8::External> wrapped = v8::External::New(object);
      result->SetInternalField(0, wrapped);
      return handle_scope.Close(result);
    }

    template<class T>
    static v8::Handle<v8::Object> wrap(T * object, v8::Persistent<v8::ObjectTemplate> templ, v8::Isolate * isolate = v8::Isolate::GetCurrent())
    {
      v8::HandleScope handle_scope(isolate);
      v8::Handle<T> temp = PersistentToHandle(templ);
      v8::Handle<v8::Object> result = temp->NewInstance();
      v8::Handle<v8::External> wrapped = v8::External::New(object);
      result->SetInternalField(0, wrapped);
      return handle_scope.Close(result);
    }

    template<class T>
    static T * unwrap(v8::Handle<v8::Object> object)
    {
      v8::Handle<v8::External> field = v8::Handle<v8::External>::Cast(object->GetInternalField(0));
      void * ptr = field->Value();
      return reinterpret_cast<T *>(ptr);
    }

  };

  class TemplateBuilder
  {
  public:
    TemplateBuilder(v8::Isolate * isolate = v8::Isolate::GetCurrent());

    TemplateBuilder & accessor(
      const std::string & name,
      v8::AccessorGetterCallback getter,
      v8::AccessorSetterCallback setter = nullptr,
      v8::Handle<v8::Value> data = v8::Handle<v8::Value>(),
      v8::AccessControl settings = v8::DEFAULT,
      v8::PropertyAttribute attribute = v8::None,
      v8::Handle<v8::AccessorSignature> signature = v8::Handle<v8::AccessorSignature>());

    TemplateBuilder & indexedPropertyHandler(
      v8::IndexedPropertyGetterCallback getter,
      v8::IndexedPropertySetterCallback setter = nullptr,
      v8::IndexedPropertyQueryCallback query = nullptr,
      v8::IndexedPropertyDeleterCallback deleter = nullptr,
      v8::IndexedPropertyEnumeratorCallback enumerator = nullptr,
      v8::Handle<v8::Value> data = v8::Handle<v8::Value>());

    v8::Handle<v8::ObjectTemplate> getTemplate();
    void getTemplate(v8::Persistent<v8::ObjectTemplate> & out);

  private:
    v8::Persistent<v8::ObjectTemplate> myTemplate;
    v8::Isolate * myIsolate;
  };
}

#endif // SCRIPTING_HPP
