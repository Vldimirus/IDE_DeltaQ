#ifndef SQLITE_DELTAQ_RUNTIME_H
#define SQLITE_DELTAQ_RUNTIME_H

#include <dlfcn.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
typedef int (*dq_sqlite_callback_fn)(void *, int, char **, char **);
typedef void (*dq_sqlite_destructor_fn)(void *);

enum {
    DQ_SQLITE_OK = 0,
    DQ_SQLITE_ROW = 100,
    DQ_SQLITE_DONE = 101
};

#define DQ_SQLITE_TRANSIENT ((dq_sqlite_destructor_fn) -1)

typedef struct {
    int init_attempted;
    int available;
    void *library_handle;
    int (*open)(const char *, sqlite3 **);
    int (*close)(sqlite3 *);
    int (*exec)(sqlite3 *, const char *, dq_sqlite_callback_fn, void *, char **);
    int (*prepare_v2)(sqlite3 *, const char *, int, sqlite3_stmt **, const char **);
    int (*bind_int)(sqlite3_stmt *, int, int);
    int (*bind_text)(sqlite3_stmt *, int, const char *, int, dq_sqlite_destructor_fn);
    int (*step)(sqlite3_stmt *);
    int (*column_int)(sqlite3_stmt *, int);
    const unsigned char *(*column_text)(sqlite3_stmt *, int);
    int (*finalize)(sqlite3_stmt *);
    void (*free_fn)(void *);
} dq_sqlite_api;

/* Возвращает singleton-состояние динамически загружаемого SQLite ABI. */
static dq_sqlite_api *dq_sqlite_api_state(void)
{
    static dq_sqlite_api api;
    return &api;
}

/* Проверяет, что все необходимые SQLite symbols найдены в уже открытой библиотеке. */
static int dq_sqlite_try_bind_symbols(dq_sqlite_api *api)
{
    if (!api || !api->library_handle)
        return 0;

    api->open = (int (*)(const char *, sqlite3 **)) dlsym(api->library_handle, "sqlite3_open");
    api->close = (int (*)(sqlite3 *)) dlsym(api->library_handle, "sqlite3_close");
    api->exec = (int (*)(sqlite3 *, const char *, dq_sqlite_callback_fn, void *, char **))
        dlsym(api->library_handle, "sqlite3_exec");
    api->prepare_v2 =
        (int (*)(sqlite3 *, const char *, int, sqlite3_stmt **, const char **))
            dlsym(api->library_handle, "sqlite3_prepare_v2");
    api->bind_int = (int (*)(sqlite3_stmt *, int, int)) dlsym(api->library_handle, "sqlite3_bind_int");
    api->bind_text =
        (int (*)(sqlite3_stmt *, int, const char *, int, dq_sqlite_destructor_fn))
            dlsym(api->library_handle, "sqlite3_bind_text");
    api->step = (int (*)(sqlite3_stmt *)) dlsym(api->library_handle, "sqlite3_step");
    api->column_int =
        (int (*)(sqlite3_stmt *, int)) dlsym(api->library_handle, "sqlite3_column_int");
    api->column_text =
        (const unsigned char *(*)(sqlite3_stmt *, int))
            dlsym(api->library_handle, "sqlite3_column_text");
    api->finalize = (int (*)(sqlite3_stmt *)) dlsym(api->library_handle, "sqlite3_finalize");
    api->free_fn = (void (*)(void *)) dlsym(api->library_handle, "sqlite3_free");

    return api->open
        && api->close
        && api->exec
        && api->prepare_v2
        && api->bind_int
        && api->bind_text
        && api->step
        && api->column_int
        && api->column_text
        && api->finalize
        && api->free_fn;
}

/* Пытается открыть доступную runtime SQLite library без зависимости на dev headers. */
static int dq_sqlite_try_open_library(dq_sqlite_api *api)
{
    static const char *const libraryNames[] = {
        "libsqlite3.so.0",
        "libsqlite3.so"
    };
    size_t i = 0U;

    if (!api)
        return 0;

    for (i = 0U; i < sizeof(libraryNames) / sizeof(libraryNames[0]); ++i) {
        api->library_handle = dlopen(libraryNames[i], RTLD_NOW | RTLD_LOCAL);
        if (api->library_handle && dq_sqlite_try_bind_symbols(api))
            return 1;
        if (api->library_handle) {
            dlclose(api->library_handle);
            api->library_handle = NULL;
        }
    }

    return 0;
}

/* Ленивая инициализация SQLite ABI для всех helper-ов pack-а. */
static int dq_sqlite_ensure_api(void)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (api->init_attempted)
        return api->available;

    api->init_attempted = 1;
    api->available = dq_sqlite_try_open_library(api);
    return api->available;
}

/* Открывает SQLite database по пути и возвращает opaque sqlite3 handle. */
static sqlite3 *dq_sqlite_open_database(const char *path)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    sqlite3 *database = NULL;

    if (!path || !*path)
        return NULL;
    if (!dq_sqlite_ensure_api())
        return NULL;
    if (api->open(path, &database) != DQ_SQLITE_OK) {
        if (database)
            api->close(database);
        return NULL;
    }
    return database;
}

/* Закрывает SQLite database handle и делает cleanup безопасным для NULL. */
static int dq_sqlite_close_database(sqlite3 *database)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (!database)
        return DQ_SQLITE_OK;
    if (!dq_sqlite_ensure_api())
        return -1;
    return api->close(database);
}

/* Выполняет произвольный SQL без result-set и возвращает SQLite status code. */
static int dq_sqlite_exec_sql(sqlite3 *database, const char *sql)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    char *errorText = NULL;
    int status = -1;

    if (!database || !sql || !*sql)
        return -1;
    if (!dq_sqlite_ensure_api())
        return -1;

    status = api->exec(database, sql, NULL, NULL, &errorText);
    if (errorText) {
        api->free_fn(errorText);
        errorText = NULL;
    }
    return status;
}

/* Подготавливает SQL statement и возвращает status code, а handle пишет в out-параметр. */
static int dq_sqlite_prepare_statement(sqlite3 *database,
                                       const char *sql,
                                       sqlite3_stmt **statement)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (statement)
        *statement = NULL;
    if (!database || !sql || !*sql || !statement)
        return -1;
    if (!dq_sqlite_ensure_api())
        return -1;
    return api->prepare_v2(database, sql, -1, statement, NULL);
}

/* Привязывает integer-параметр к prepared statement по 1-based индексу. */
static int dq_sqlite_bind_statement_int(sqlite3_stmt *statement, int index, int value)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (!statement)
        return -1;
    if (!dq_sqlite_ensure_api())
        return -1;
    return api->bind_int(statement, index, value);
}

/* Привязывает text-параметр к prepared statement с transient SQLite semantics. */
static int dq_sqlite_bind_statement_text(sqlite3_stmt *statement, int index, const char *value)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (!statement)
        return -1;
    if (!dq_sqlite_ensure_api())
        return -1;
    return api->bind_text(statement, index, value ? value : "", -1, DQ_SQLITE_TRANSIENT);
}

/* Двигает statement на один шаг вперёд и возвращает raw SQLite step status. */
static int dq_sqlite_step_statement(sqlite3_stmt *statement)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (!statement)
        return -1;
    if (!dq_sqlite_ensure_api())
        return -1;
    return api->step(statement);
}

/* Читает integer-колонку из текущей строки prepared statement. */
static int dq_sqlite_column_int_value(sqlite3_stmt *statement, int columnIndex)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (!statement)
        return 0;
    if (!dq_sqlite_ensure_api())
        return 0;
    return api->column_int(statement, columnIndex);
}

/* Читает text-колонку из текущей строки и копирует её в стабильный внутренний буфер. */
static const char *dq_sqlite_column_text_value(sqlite3_stmt *statement, int columnIndex)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    static char buffer[2048];
    const unsigned char *text = NULL;

    buffer[0] = '\0';
    if (!statement)
        return buffer;
    if (!dq_sqlite_ensure_api())
        return buffer;

    text = api->column_text(statement, columnIndex);
    if (!text)
        return buffer;
    snprintf(buffer, sizeof(buffer), "%s", (const char *) text);
    return buffer;
}

/* Освобождает prepared statement и делает finalize безопасным для NULL. */
static int dq_sqlite_finalize_statement(sqlite3_stmt *statement)
{
    dq_sqlite_api *api = dq_sqlite_api_state();
    if (!statement)
        return DQ_SQLITE_OK;
    if (!dq_sqlite_ensure_api())
        return -1;
    return api->finalize(statement);
}

#endif
