/* seal_ailines.cpp -- the AI's debug lines: DrawLine, LineTo, MoveTo, the line colour stack (SOCOM 1's
 * unit, __sinit_seal_ailines.cpp, after seal_squirm.cpp). OVL2 from ~0x5FF2F0 (after CAiTrigger's
 * SetTimes, seal_squirm.cpp's last) to before seal_look.cpp; ai::MoveTo (0x600070) is SOCOM 1's,
 * byte-exact as a draft. So far the line-type test. Defined in reverse of their image order
 * (-inline deferred). */

namespace ai {

/* a line's kind, a bit each (SOCOM 1's ai::LINE_TYPE; the values SOCOM II's callers pass -- 512 for
 * the follow state's -- are masks, used unextended: a word) */
enum LINE_TYPE { LINE_ANY = 0, LINE_TYPE_LAST = 0x7fffffff };

/* the kinds of line drawn (SOCOM 1's ai_line_mask; OVL2 bss 0x66BB10) */
extern unsigned int ai_line_mask;

/* whether a kind of line is drawn: any line when given none (OVL2 0x5FFF10, new in SOCOM II --
 * SOCOM 1's DrawLine tested it inline, as SOCOM II's still does; CAiSFollow::Tick calls this copy;
 * the name ours) */
bool LineOn(LINE_TYPE type)
{
    return type == LINE_ANY || (ai_line_mask & type) != 0;
}

}   // namespace ai
