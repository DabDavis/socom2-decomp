/* mp_rating.cpp -- the network game's player ratings (OVL1 0x239770-0x23A2DF): SOCOM II's unit, no
 * SOCOM 1 counterpart (the file name and every name in it are ours). Between hud_mpinfobox.cpp's
 * tail (basic_string(const char *), 0x2396F0) and the voice recogniser's unit (0x23A2E0, reco.cpp).
 * An Elo rating over two teams: each player's expected share of the score, 1 / (1 + 10^((Rb -
 * Ra) / 400)), the new rating Ra + K (S - E), floored at 200 -- CMPExitState's 0x2253C0 builds a
 * match (CRatingMatch(), AddTeam, AddPlayer), computes it and reads each player's new rating back.
 * Defined in reverse of their image order (-inline deferred). */

#include <list>

extern "C" {
float fabsf(float x);
double pow(double x, double y);
}

/* a player in a team (0x18 bytes) */
class CRatingPlayer {
public:
    CRatingPlayer(int player, int team, int score, float rating)
    {
        m_player = player;
        m_team = team;
        m_score = score;
        m_rating = rating;
    }

    int m_player;   // 0x0: whom the rating is
    int m_team;   // 0x4: the team's id
    int m_score;   // 0x8
    float m_rating;   // 0xc: the rating before the game
    float m_share;   // 0x10: the share of the team's result
    float m_new;   // 0x14: the rating after it
};   // 0x18

/* a team (0x24 bytes): its players, its id, its average rating */
class CRatingTeam {
public:
    CRatingTeam(int id, int count, bool first, float rating)
    {
        m_id = id;
        m_rating = rating;
        m_count = count;
        m_first = first;
    }
    ~CRatingTeam();

    /* a player added (by value, so the pointer stays in a register: only push_back's copy is
     * stored, as AddPlayer's `sw s0` in the call's delay slot) */
    void Add(CRatingPlayer *player) { m_players.push_back(player); }

    bool GetNewRating(int player, float *rating);
    float Update(bool solo, int k, float opponent, float score);
    float SetShares(float score);
    void SetAverage();
    void SumScores();

    std::list<CRatingPlayer *> m_players;   // 0x0
    int m_id;   // 0xc
    float m_rating;   // 0x10: the average rating
    int m_count;   // 0x14
    bool m_first;   // 0x18
    int m_score;   // 0x1c: the players' scores summed
    float m_new;   // 0x20: the team's rating after the game (one player a side)
};   // 0x24

/* a match: two teams */
class CRatingMatch {
public:
    CRatingMatch();
    ~CRatingMatch();

    CRatingTeam *AddTeam(int id, int count, bool first, float rating);
    int AddPlayer(int team, int player, int score, float rating);
    float Compute();
    bool GetNewRating(int player, float *rating);

    CRatingTeam *m_teams[2];   // 0x0
    bool m_solo;   // 0x8: one player a side
    bool m_done;   // 0x9: computed since the last change
    int m_rounds;   // 0xc: the game's length, which weights the draw
    int m_k;   // 0x10: the rating's step
};

CRatingMatch::CRatingMatch()
{
    m_teams[0] = 0;
    m_teams[1] = 0;
    m_solo = false;
    m_done = false;
}

CRatingMatch::~CRatingMatch()
{
    if (m_teams[0])
        delete m_teams[0];
    if (m_teams[1])
        delete m_teams[1];
}

/* the new team, or none when both slots are taken (round 56: it RETURNS the team -- the target keeps
 * `team` in v0 from the entry's `move v0,zero` to the epilogue. Returned void, MW coloured it a0 and
 * the parameters, at exactly K = 25 neighbours (id's with team and the slot test's load), were
 * deferred and took s0-s2 where the target's have s5-s3: the round-19 "parameters numbered last" was
 * that deferral -- `tools/igreplay.py --drop 33-36` gave the target's registers before the source did;
 * the caller ignores the result, and a return type is not mangled) */
CRatingTeam *CRatingMatch::AddTeam(int id, int count, bool first, float rating)
{
    CRatingTeam *team = 0;
    int i;

    m_done = false;
    for (i = 0; i < 2 && !team; i++) {
        if (!m_teams[i]) {
            m_teams[i] = new CRatingTeam(id, count, first, rating);
            team = m_teams[i];
        }
    }
    return team;
}

int CRatingMatch::AddPlayer(int team, int player, int score, float rating)
{
    CRatingTeam *t = 0;
    int i;

    m_done = false;
    for (i = 0; i < 2 && !t; i++) {
        if (m_teams[i] && team == m_teams[i]->m_id)
            t = m_teams[i];
    }
    if (t) {
        bool ok = true;
        CRatingPlayer *pl = new CRatingPlayer(player, team, score, rating);

        if (pl)
            t->Add(pl);
        else
            ok = false;
        if (!ok && pl)
            delete pl;
        return ok;
    }
    return false;
}

float CRatingMatch::Compute()
{
    int a;
    int b;
    int players;
    float half;
    float sa, sb;
    float change;

    if (!m_teams[0] || !m_teams[1])
        return 0.0f;
    a = 0;
    if (m_teams[1]->m_first)
        a = 1;
    b = (a + 1) % 2;
    if (!m_solo) {
        m_teams[0]->SumScores();
        m_teams[1]->SumScores();
        m_teams[0]->SetAverage();
        m_teams[1]->SetAverage();
    }
    players = m_teams[0]->m_players.size() + m_teams[1]->m_players.size();
    half = 0.5f * players;
    if (m_solo) {
        sa = 1.0f;
        sb = 0.0f;
    } else {
        float w;

        switch (m_rounds) {
        case 3:
            w = 0.35f;
            break;
        case 5:
            w = 0.16f;
            break;
        case 7:
            w = 0.1f;
            break;
        case 9:
            w = 0.07f;
            break;
        case 11:
            w = 0.05f;
            break;
        default:
            w = 0.05f;
            break;
        }
        sb = w * (half * m_teams[b]->m_count);
        sa = half - sb;
    }
    if (!m_solo) {
        float total = 0.0f;

        total += m_teams[a]->SetShares(sa);
        total += m_teams[b]->SetShares(sb);
        fabsf(total - half);
    }
    if (m_solo)
        m_k = 50;
    else
        m_k = players * 3;
    change = 0.0f;
    change += m_teams[a]->Update(m_solo, m_k, m_teams[b]->m_rating, sa);
    change += m_teams[b]->Update(m_solo, m_k, m_teams[a]->m_rating, sb);
    fabsf(change);
    m_done = true;
    return change;
}

bool CRatingMatch::GetNewRating(int player, float *rating)
{
    bool found = false;

    if (rating) {
        int i;

        for (i = 0; found == false && i < 2; i++) {
            if (m_teams[i] && m_teams[i]->GetNewRating(player, rating))
                found = true;
        }
    }
    return found;
}

/* the team's functions, every one called out of line by the match's (pins) */
#pragma auto_inline off
CRatingTeam::~CRatingTeam()
{
    std::list<CRatingPlayer *>::iterator i;

    i = m_players.begin();
    std::list<CRatingPlayer *>::iterator e = m_players.end();
    for (; i != e; ++i)
        delete *i;
    m_players.clear();
}

void CRatingTeam::SumScores()
{
    std::list<CRatingPlayer *>::iterator i;

    m_score = 0;
    i = m_players.begin();
    std::list<CRatingPlayer *>::iterator e = m_players.end();
    for (; i != e; ++i)
        m_score += (*i)->m_score;
}

void CRatingTeam::SetAverage()
{
    m_rating = 0.0f;
    if (m_players.size()) {
        std::list<CRatingPlayer *>::iterator i;

        i = m_players.begin();
    std::list<CRatingPlayer *>::iterator e = m_players.end();
    for (; i != e; ++i)
            m_rating += (*i)->m_rating;
        m_rating /= m_players.size();
    }
}

float CRatingTeam::SetShares(float score)
{
    std::list<CRatingPlayer *>::iterator i;
    float total = 0.0f;

    i = m_players.begin();
    std::list<CRatingPlayer *>::iterator e = m_players.end();
    for (; i != e; ++i) {
        int sum = m_score;
        int n = m_players.size();
        bool first = m_first;
        CRatingPlayer *p = *i;

        if (sum > 0) {
            if (first)
                p->m_share = 0.5f + p->m_score * (score - 0.5f * n) / m_score;
            else
                p->m_share = score * p->m_score / m_score;
        } else
            p->m_share = 0.0f;
        total += p->m_share;
    }
    return total;
}

float CRatingTeam::Update(bool solo, int k, float opponent, float score)
{
    float expected = 1.0 / (1.0 + pow(10.0, (opponent - m_rating) / 400.0f));
    float change = 0.0f;

    if (solo) {
        float r = m_rating + k * (score - expected);

        if (r < 200.0f)
            r = 200.0f;
        m_new = r;
        change = r - m_rating;
    } else {
        std::list<CRatingPlayer *>::iterator i;

        i = m_players.begin();
    std::list<CRatingPlayer *>::iterator e = m_players.end();
    for (; i != e; ++i) {
            CRatingPlayer *p = *i;
            float r = p->m_rating + k * (p->m_share - expected);

            if (r < 200.0f)
                r = 200.0f;
            p->m_new = r;
            change += r - p->m_rating;
        }
    }
    return change;
}

bool CRatingTeam::GetNewRating(int player, float *rating)
{
    if (rating) {
        std::list<CRatingPlayer *>::iterator i;

        i = m_players.begin();
        std::list<CRatingPlayer *>::iterator e = m_players.end();
        for (; i != e; ++i) {
            if (player == (*i)->m_player) {
                *rating = (*i)->m_new;
                return true;
            }
        }
    }
    return false;
}
#pragma auto_inline reset
