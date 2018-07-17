#ifndef CHIMERA_DATABASE_HPP
#define CHIMERA_DATABASE_HPP

#include <ch.h>
#include <nan.h>
#include <vector>

class ChimeraDatabase : public Nan::ObjectWrap
{
public:
    ChimeraDatabase(std::vector<std::string> patterns);
    ~ChimeraDatabase();

    static int ScanEventHandler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags, unsigned int size, const ch_capture_t *captured, void *context);

    static NAN_MODULE_INIT(Init);
    static NAN_METHOD(New);
    static NAN_METHOD(Scan);

    std::vector<std::string> m_patterns;
    std::vector<std::tuple<unsigned int, unsigned long long, unsigned long long, std::vector<std::tuple<unsigned long long, unsigned long long>>>> m_scanMatches;

    static ch_scratch_t *s_scratch;
    ch_database_t *m_database;

    static Nan::Persistent<v8::FunctionTemplate> constructor;
};

#endif
