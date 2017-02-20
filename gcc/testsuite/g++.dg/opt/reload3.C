// PR target/38287
// { dg-do run { target { stdint_types } } }
// { dg-options "-O2 -mcpu=v8 -fPIC" { target { { sparc*-*-* } && { ilp32 && fpic } } } }

#include <cstdlib>
#include <stdint.h>

class QTime
{
public:
    explicit QTime(int_least32_t ms = 0) : ds(ms) {}
    static QTime currentTime() { return QTime(); }
    QTime addMSecs(int_least32_t ms) const;
    int_least32_t msecs() const { return ds; }
private:
    uint_least32_t ds;
};

static const uint_least32_t MSECS_PER_DAY = 86400000;

QTime QTime::addMSecs(int_least32_t ms) const
{
    QTime t;
    if ( ms < 0 ) {
        // % not well-defined for -ve, but / is.
        int_least32_t negdays = (MSECS_PER_DAY-ms) / MSECS_PER_DAY;
        t.ds = ((int_least32_t)ds + ms + negdays*MSECS_PER_DAY)
                % MSECS_PER_DAY;
    } else {
        t.ds = ((int_least32_t)ds + ms) % MSECS_PER_DAY;
    }
    return t;
}

int main()
{
  if (QTime(1).addMSecs(1).msecs() != 2)
    abort ();
  return 0;
}
