#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <errno.h>
#include <math.h>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_true(lept_context* c, lept_value* v) {
    EXPECT(c, 't');
    if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_TRUE;
    return LEPT_PARSE_OK;
}

static int lept_parse_false(lept_context* c, lept_value* v) {
    EXPECT(c, 'f');
    if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 4;
    v->type = LEPT_FALSE;
    return LEPT_PARSE_OK;
}

static int lept_parse_null(lept_context* c, lept_value* v) {
    EXPECT(c, 'n');
    if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
        return LEPT_PARSE_INVALID_VALUE;
    c->json += 3;
    v->type = LEPT_NULL;
    return LEPT_PARSE_OK;
}

static int lept_parse_literal(lept_context* c, lept_value* v) {
    
    char* literals[3];
    int types[3];
    int i;
    literals[0] = "null";
    literals[1] = "false";
    literals[2] = "true";
    types[0] = LEPT_NULL;
    types[1] = LEPT_FALSE;
    types[2] = LEPT_TRUE;
    
    for (i = 0; i < 3; i++) {
        if (literals[i][0] == c->json[0]) {
            int j = 1;
            while (literals[i][j] != '\0') {
                if (literals[i][j] != c->json[j]) {
                    return LEPT_PARSE_INVALID_VALUE;
                }
                j++;
            }
            v->type = types[i];
            c->json += j;
            return LEPT_PARSE_OK;
        }
    }
    return LEPT_PARSE_INVALID_VALUE;
}

/*answer 1: refactor */

static int lept_parse_literal_1(lept_context*c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++) {
        if (c->json[i] != literal[i + 1]) {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_set_tod_result(lept_context* c, lept_value* v, int j_offset) {
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE) {
        errno = 0;
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    c->json += j_offset;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    int i;

    i = 0;

    /*parse sign*/
    if (!ISDIGIT(c->json[i])) {
        if (c->json[i] == '-') {
            i++;
        } else {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }

    /*0 follow check*/
    if (c->json[i] == '0') {
        if (c->json[i+1] != '\0' && c->json[i+1] != '.' && c->json[i+1] != 'e' && c->json[i+1] != 'E') {
            i += 1;
            return lept_set_tod_result(c, v, i);
        }
    }
    
    while (c->json[i] != '\0') {
        
        /* . follow check */
        if (c->json[i] == '.') {
            if (!ISDIGIT(c->json[i+1])) {
                return LEPT_PARSE_INVALID_VALUE;
            } else {
                i++;
                while (ISDIGIT(c->json[i])) {
                    i++;
                }
                if (c->json[i] != 'e' && c->json[i] != 'E') {
                    break;
                }
            }
        }
        
        /* e follow check */
        if (c->json[i] == 'e' || c->json[i] == 'E') {
            if (c->json[i+1] != '-' && c->json[i+1] != '+') {
                i += 1;
                if(!ISDIGIT(c->json[i])) {
                    return LEPT_PARSE_INVALID_VALUE;
                }
            } else {
                i += 2;
                if(!ISDIGIT(c->json[i])) {
                    return LEPT_PARSE_INVALID_VALUE;
                }
            }
            while (ISDIGIT(c->json[i])) {
                i++;
            }
            break;
        }
        i++;
    }
    
    return lept_set_tod_result(c, v, i);
}

static int lept_parse_number_1(lept_context* c, lept_value* v) {
    const char* p = c->json;
    
    if (*p == '-') p++;
    
    if (*p == '0') p++;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        for (p++; ISDIGIT(*p); p++);
    }
    
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE || v->n == HUGE_VAL) return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    c->json = p;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
/*        case 't':  return lept_parse_true(c, v);
        case 'f':  return lept_parse_false(c, v);
        case 'n':  return lept_parse_null(c, v); */
//        case 't': case 'f': case 'n': return lept_parse_literal(c, v);
        case 't': return lept_parse_literal_1(c, v, "true", LEPT_TRUE);
        case 'f': return lept_parse_literal_1(c, v, "false", LEPT_FALSE);
        case 'n': return lept_parse_literal_1(c, v, "null", LEPT_NULL);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
            
        default:   return lept_parse_number_1(c, v);
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
