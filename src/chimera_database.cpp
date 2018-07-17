#include <node_buffer.h>

#include "chimera_database.hpp"

ch_scratch_t *ChimeraDatabase::s_scratch = nullptr;
Nan::Persistent<v8::FunctionTemplate> ChimeraDatabase::constructor;

struct PatternOptions
{
    PatternOptions() : caseless(0), dotAll(0), multiline(0), singleMatch(0), utf8(0), ucp(0)
    {
    }

    PatternOptions(bool caseless, bool dotAll, bool multiline, bool singleMatch, bool utf8, bool ucp) : PatternOptions()
    {
        if(caseless == true)
        {
            this->caseless = CH_FLAG_CASELESS;
        }

        if(dotAll == true)
        {
            this->dotAll = CH_FLAG_DOTALL;
        }

        if(multiline == true)
        {
            this->multiline =  CH_FLAG_MULTILINE;
        }

        if(singleMatch == true)
        {
            this->singleMatch = CH_FLAG_SINGLEMATCH;
        }

        if(utf8 == true)
        {
            this->utf8 = CH_FLAG_UTF8;
        }

        if(ucp == true)
        {
            this->ucp = CH_FLAG_UCP;
        }
    }

    unsigned int caseless;
    unsigned int dotAll;
    unsigned int multiline;
    unsigned int singleMatch;
    unsigned int utf8;
    unsigned int ucp;
};

struct CompileOptions
{
    CompileOptions() : captureGroups(CH_MODE_GROUPS)
    {
    }

    CompileOptions(bool captureGroups) : CompileOptions()
    {
        if(captureGroups == false)
        {
            this->captureGroups = CH_MODE_NOGROUPS;
        }
    }

    unsigned int captureGroups;
};

ChimeraDatabase::ChimeraDatabase(std::vector<std::string> patterns) : m_patterns(patterns), m_scanMatches(), m_database(nullptr)
{
}

ChimeraDatabase::~ChimeraDatabase()
{
}

int ChimeraDatabase::ScanEventHandler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, unsigned int size, const ch_capture_t *capture, void *context)
{
    std::vector<std::tuple<unsigned long long, unsigned long long>> subMatches;
    for(size_t i = 1; i < size; ++i)
    {
        subMatches.push_back(std::make_tuple((capture+i)->from, (capture+i)->to));
    }

    (static_cast<ChimeraDatabase*>(context))->m_scanMatches.push_back(std::make_tuple(id, from, to, subMatches));
    return 0;
}

NAN_MODULE_INIT(ChimeraDatabase::Init)
{
    v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(ChimeraDatabase::New);
    constructor.Reset(ctor);
    ctor->InstanceTemplate()->SetInternalFieldCount(1);
    ctor->SetClassName(Nan::New("ChimeraDatabase").ToLocalChecked());

    Nan::SetPrototypeMethod(ctor, "scan", Scan);

    target->Set(Nan::New("ChimeraDatabase").ToLocalChecked(), ctor->GetFunction());
}

NAN_METHOD(ChimeraDatabase::New)
{
    // Veryify types
    if(!info.IsConstructCall())
    {
        return Nan::ThrowError(Nan::New("ChimeraDatabase::New - called without new keyword").ToLocalChecked());
    }

    if(info.Length() < 1 || info.Length() > 3)
    {
        return Nan::ThrowError(Nan::New("ChimeraDatabase::New - unexpected or missing arguments").ToLocalChecked());
    }

    if(!info[0]->IsArray())
    {
        return Nan::ThrowError(Nan::New("ChimeraDatabase::New - first argument is not an array").ToLocalChecked());
    }

    if(!info[1]->IsArray())
    {
        return Nan::ThrowError(Nan::New("ChimeraDatabase::New - second argument is not an array").ToLocalChecked());
    }

    // Convert node array of node strings into a vector of strings
    v8::Local<v8::Array> nodePatterns = v8::Local<v8::Array>::Cast(info[0]);
    std::vector<std::string> patterns;
    patterns.reserve(nodePatterns->Length());
    for(size_t i = 0; i < nodePatterns->Length(); ++i)
    {
        if(!nodePatterns->Get(i)->IsString())
        {
            return Nan::ThrowTypeError("ChimeraDatabase::New - expected string in pattern array");
        }

        Nan::Utf8String nodePattern(nodePatterns->Get(i));
        patterns.push_back(std::string(*nodePattern, nodePattern.length()));
    }

    // Convert node array of pattern options into a vector of pattern options
    v8::Local<v8::Array> nodeOptions = v8::Local<v8::Array>::Cast(info[1]);
    std::vector<PatternOptions> patternOptions = std::vector<PatternOptions>();
    patternOptions.reserve(nodeOptions->Length());
    if(info.Length() >= 2)
    {
        for(size_t i = 0; i < nodeOptions->Length(); ++i)
        {
            if(!nodeOptions->Get(i)->IsObject())
            {
                return Nan::ThrowTypeError("ChimeraDatabase::New - expected object in pattern options array");
            }

            v8::Local<v8::Object> option = nodeOptions->Get(i)->ToObject();

            bool caseless = false;
            v8::Local<v8::String> caselessKey = Nan::New("caseless").ToLocalChecked();
            if(option->HasOwnProperty(Nan::GetCurrentContext(), caselessKey).FromMaybe(false))
            {
                v8::Local<v8::Value> caselessValue = option->Get(caselessKey);
                if (caselessValue->IsBoolean())
                {
                    caseless = caselessValue->BooleanValue(Nan::GetCurrentContext()).FromMaybe(false);
                }
            }

            bool dotAll = false;
            v8::Local<v8::String> dotAllKey = Nan::New("dotAll").ToLocalChecked();
            if(option->HasOwnProperty(Nan::GetCurrentContext(), dotAllKey).FromMaybe(false))
            {
                v8::Local<v8::Value> dotAllValue = option->Get(dotAllKey);
                if (dotAllValue->IsBoolean())
                {
                    dotAll = dotAllValue->BooleanValue(Nan::GetCurrentContext()).FromMaybe(false);
                }
            }

            bool multiline = false;
            v8::Local<v8::String> multilineKey = Nan::New("multiline").ToLocalChecked();
            if(option->HasOwnProperty(Nan::GetCurrentContext(), multilineKey).FromMaybe(false))
            {
                v8::Local<v8::Value> multilineValue = option->Get(multilineKey);
                if (multilineValue->IsBoolean())
                {
                    multiline = multilineValue->BooleanValue(Nan::GetCurrentContext()).FromMaybe(false);
                }
            }

            bool singleMatch = false;
            v8::Local<v8::String> singleMatchKey = Nan::New("singleMatch").ToLocalChecked();
            if(option->HasOwnProperty(Nan::GetCurrentContext(), singleMatchKey).FromMaybe(false))
            {
                v8::Local<v8::Value> singleMatchValue = option->Get(singleMatchKey);
                if (singleMatchValue->IsBoolean())
                {
                    singleMatch = singleMatchValue->BooleanValue(Nan::GetCurrentContext()).FromMaybe(false);
                }
            }

            bool utf8 = false;
            v8::Local<v8::String> utf8Key = Nan::New("utf8").ToLocalChecked();
            if(option->HasOwnProperty(Nan::GetCurrentContext(), utf8Key).FromMaybe(false))
            {
                v8::Local<v8::Value> utf8Value = option->Get(utf8Key);
                if (utf8Value->IsBoolean())
                {
                    utf8 = utf8Value->BooleanValue(Nan::GetCurrentContext()).FromMaybe(false);
                }
            }

            bool ucp = false;
            v8::Local<v8::String> ucpKey = Nan::New("ucp").ToLocalChecked();
            if(option->HasOwnProperty(Nan::GetCurrentContext(), ucpKey).FromMaybe(false))
            {
                v8::Local<v8::Value> ucpValue = option->Get(ucpKey);
                if (ucpValue->IsBoolean())
                {
                    ucp = ucpValue->BooleanValue(Nan::GetCurrentContext()).FromMaybe(false);
                }
            }

            patternOptions.push_back(PatternOptions(caseless, dotAll, multiline, singleMatch, utf8, ucp));
        }
    }

    // If some pattern patternOptions are missing we fill them in
    for (size_t i = patternOptions.size(); i < patterns.size(); ++i)
    {
        patternOptions.push_back(PatternOptions());
    }

    // Create object and bind it to node object
    ChimeraDatabase *obj = new ChimeraDatabase(patterns);
    obj->Wrap(info.Holder());

    // Convert patterns from a vector of strings to a const char * const *
    std::vector<const char*> cStrings;
    std::vector<unsigned int> ids;
    std::vector<unsigned int> flags;

    cStrings.reserve(patterns.size());
    ids.reserve(patterns.size());
    flags.reserve(patterns.size());

    for(size_t i = 0; i < patterns.size(); ++i) {
        cStrings.push_back(patterns[i].c_str());
        ids.push_back(i);
        PatternOptions option = patternOptions[i];
        flags.push_back((option.caseless & CH_FLAG_CASELESS) | (option.dotAll & CH_FLAG_DOTALL) | (option.multiline & CH_FLAG_MULTILINE) | (option.singleMatch & CH_FLAG_SINGLEMATCH) | (option.utf8 & CH_FLAG_UTF8) | (option.ucp & CH_FLAG_UCP));
    }

    // Compile the patterns into a database
    ch_compile_error_t *compileError;
    if(ch_compile_multi(cStrings.data(), flags.data(), ids.data(), cStrings.size(), CH_MODE_GROUPS, NULL, &obj->m_database, &compileError) != CH_SUCCESS)
    {
        ch_free_compile_error(compileError);
        return Nan::ThrowTypeError(std::string("ChimeraDatabase::New - failed to compile pattern database: ").append(std::string(compileError->message)).c_str());
    }

    // Create scratch space that hyperscan uses while scanning
    if(ch_alloc_scratch(obj->m_database, &ChimeraDatabase::s_scratch) != CH_SUCCESS)
    {
        ch_free_database(obj->m_database);
        return Nan::ThrowTypeError("ChimeraDatabase::New - failed to allocate scratch space for hyperscan");
    }

    // Return newly created object
    info.GetReturnValue().Set(info.Holder());
}

NAN_METHOD(ChimeraDatabase::Scan)
{
    int argLength = info.Length();

    // Get the object that is having scan invoked on
    ChimeraDatabase *self = Nan::ObjectWrap::Unwrap<ChimeraDatabase>(info.This());

    // Check input types
    if(argLength != 1 && argLength != 2)
    {
        return Nan::ThrowTypeError("ChimeraDatabase::Scan - unexpected number of arguments");
    }

    const char* rawInput;
    size_t rawInputLength;
    if (info[0]->IsString())
    {
        Nan::Utf8String nodeInput(info[0]);
        rawInputLength = nodeInput.length();
        rawInput = *nodeInput;
    }
    else if (info[0]->IsUint8Array())
    {
        v8::Local<v8::Object> bufferObj = info[0]->ToObject();
        rawInput = node::Buffer::Data(bufferObj);
        rawInputLength = node::Buffer::Length(bufferObj);
    }
    else
    {
        return Nan::ThrowTypeError("ChimeraDatabase::Scan - expected string or buffer as first argument");
    }

    // Do the search with hyperscan
    if(ch_scan(self->m_database, rawInput, rawInputLength, 0, ChimeraDatabase::s_scratch, ChimeraDatabase::ScanEventHandler, NULL, static_cast<void*>(self)) != CH_SUCCESS)
    {
        ch_free_scratch(ChimeraDatabase::s_scratch);
        ch_free_database(self->m_database);
        return Nan::ThrowTypeError("ChimeraDatabase::Scan - hyperscan failed to scan input string");
    }

    // Return a 1d typed array
    v8::Isolate *isolate = info.GetIsolate();
    unsigned int arraySize = 4 * self->m_scanMatches.size();
    // Iterate matches and calculate sub expression matches
    for(const auto& match : self->m_scanMatches)
    {
        arraySize += 2 * std::get<3>(match).size();
    }
    v8::Local<v8::Uint32Array> nodeMatches = v8::Uint32Array::New(v8::ArrayBuffer::New(isolate, 4 * arraySize), 0, arraySize);
    uint32_t *ptr = *Nan::TypedArrayContents<uint32_t>(nodeMatches);
    for(const auto& match : self->m_scanMatches)
    {
        *(ptr++) = static_cast<uint32_t>(std::get<0>(match));
        *(ptr++) = static_cast<uint32_t>(std::get<1>(match));
        *(ptr++) = static_cast<uint32_t>(std::get<2>(match));
        *(ptr++) = static_cast<uint32_t>(std::get<3>(match).size());
        for(const auto& subMatch : std::get<3>(match))
        {
            *(ptr++) = static_cast<uint32_t>(std::get<0>(subMatch));
            *(ptr++) = static_cast<uint32_t>(std::get<1>(subMatch));
        }
    }
    info.GetReturnValue().Set(nodeMatches);

    self->m_scanMatches.clear();
}
