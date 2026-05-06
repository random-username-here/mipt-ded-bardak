#pragma once

#include "snapshot.hpp"
#include "AssetManager.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vis {

struct Vec2f {
    float x;
    float y;

    Vec2f()
        : x(0.0f)
        , y(0.0f)
    {}

    Vec2f(float xx, float yy)
        : x(xx)
        , y(yy)
    {}
};

enum Direction {
    DIR_DOWN = 0,
    DIR_UP = 1,
    DIR_LEFT = 2,
    DIR_RIGHT = 3
};

class DirectionUtil {
public:
    static Direction fromDelta(int dx, int dy, Direction fallback) {
        if (std::abs(dx) > std::abs(dy)) {
            return dx < 0 ? DIR_LEFT : DIR_RIGHT;
        }

        if (std::abs(dy) > 0) {
            return dy < 0 ? DIR_UP : DIR_DOWN;
        }

        return fallback;
    }

    static Direction toward(int fromX, int fromY, int toX, int toY, Direction fallback) {
        return fromDelta(toX - fromX, toY - fromY, fallback);
    }

    static Vec2f vector(Direction dir) {
        switch (dir) {
        case DIR_DOWN:  return Vec2f( 0.0f,  1.0f);
        case DIR_UP:    return Vec2f( 0.0f, -1.0f);
        case DIR_LEFT:  return Vec2f(-1.0f,  0.0f);
        case DIR_RIGHT: return Vec2f( 1.0f,  0.0f);
        }

        return Vec2f(0.0f, 1.0f);
    }

    static float rotation(Direction dir) {
        // NOTE: assume slash sprite is authored facing right

        switch (dir) {
        case DIR_RIGHT: return   0.0f;
        case DIR_DOWN:  return  90.0f;
        case DIR_LEFT:  return 180.0f;
        case DIR_UP:    return 270.0f;
        }

        return 0.0f;
    }
};

class Easing {
public:
    static float clamp01(float t) {
        return std::max(0.0f, std::min(1.0f, t));
    }

    static float smoothstep(float t) {
        t = clamp01(t);
        return t * t * (3.0f - 2.0f * t);
    }

    static float easeOutCubic(float t) {
        t = clamp01(t);
        const float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    static float sinePulse(float t) {
        t = clamp01(t);
        return static_cast<float>(std::sin(t * M_PI));
    }
};

class Timeline {
    double m_start;
    double m_end;

public:
    Timeline()
        : m_start(0.0)
        , m_end(0.0)
    {}

    Timeline(double start, double end)
        : m_start(start)
        , m_end(end)
    {}

    double start() const { return m_start; }
    double end()   const { return m_end; }

    bool active(double now) const {
        return now >= m_start && now < m_end;
    }

    float linear(double now) const {
        if (now >= m_end)   return 1.0f;
        if (now <= m_start) return 0.0f;

        const double duration = m_end - m_start;
        if (duration <= 0.0) return 1.0f;

        return Easing::clamp01(static_cast<float>((now - m_start) / duration));
    }

    float smooth(double now) const {
        return Easing::smoothstep(linear(now));
    }

    float easeOut(double now) const {
        return Easing::easeOutCubic(linear(now));
    }

    float pulse(double now) const {
        return Easing::sinePulse(linear(now));
    }
};

struct Motion {
    Vec2f from;
    Vec2f to;
    Timeline time;

    Motion()
        : from()
        , to()
        , time()
    {}

    Vec2f pos(double now) const {
        const float a = time.easeOut(now);
        return Vec2f(
            from.x + (to.x - from.x) * a,
            from.y + (to.y - from.y) * a
        );
    }

    Vec2f posSmooth(double now) const {
        const float a = time.smooth(now);
        return Vec2f(
            from.x + (to.x - from.x) * a,
            from.y + (to.y - from.y) * a
        );
    }
};

struct Corpse {
    int x;
    int y;
    size_t id;

    Corpse()
        : x(0)
        , y(0)
        , id(0)
    {}

    Corpse(int xx, int yy, size_t iid)
        : x(xx)
        , y(yy)
        , id(iid)
    {}
};

struct AssetChangeEvent {
    size_t entityID;
    modlib::AssetId assetId;

    AssetChangeEvent()
        : entityID(0)
        , assetId(modlib::kInvalidAssetId)
    {}

    AssetChangeEvent(size_t eid, modlib::AssetId aid)
        : entityID(eid)
        , assetId(aid)
    {}
};

class VisualUnit {
    size_t m_id;

    int m_x;
    int m_y;

    int m_hp;
    int m_maxHp;

    Direction m_dir;
    Motion    m_motion;

    modlib::AssetId m_assetId;

public:
    VisualUnit()
        : m_id(0)
        , m_x(0)
        , m_y(0)
        , m_hp(0)
        , m_maxHp(0)
        , m_dir(DIR_DOWN)
        , m_motion()
        , m_assetId(modlib::kInvalidAssetId)
    {}

    explicit VisualUnit(const UnitSnap &u)
        : m_id(u.id)
        , m_x(u.x)
        , m_y(u.y)
        , m_hp(u.hp)
        , m_maxHp(u.maxHp)
        , m_dir(DIR_DOWN)
        , m_motion()
        , m_assetId(u.assetId)
    {
        m_motion.from = Vec2f(static_cast<float>(u.x), static_cast<float>(u.y));
        m_motion.to = m_motion.from;
    }

    size_t id() const { return m_id; }
    int     x() const { return m_x;  }
    int     y() const { return m_y;  }

    int    hp() const { return m_hp;    }
    int maxHp() const { return m_maxHp; }

    Direction dir() const { return m_dir; }

    modlib::AssetId assetId() const { return m_assetId; }

    Vec2f renderPos(double now) const {
        return m_motion.pos(now);
    }

    void applySnapshot(const UnitSnap &u, const VisualUnit *old, double now, double tickSeconds) {
        m_id = u.id;

        m_x = u.x;
        m_y = u.y;

        m_hp    = u.hp;
        m_maxHp = u.maxHp;
        m_assetId = u.assetId;


        if (!old) {
            m_dir         = DIR_DOWN;
            m_motion.from = Vec2f(static_cast<float>(u.x), static_cast<float>(u.y));
            m_motion.to   = m_motion.from;
            m_motion.time = Timeline();
            return;
        }

        m_dir    = old->m_dir;
        m_motion = old->m_motion;
        if (m_assetId == modlib::kInvalidAssetId) {
            m_assetId = old->m_assetId;
        }


        const int dx = u.x - old->m_x;
        const int dy = u.y - old->m_y;

        if (dx != 0 || dy != 0) {
            const Vec2f cur = old->m_motion.posSmooth(now);

            m_motion.from = cur;
            m_motion.to   = Vec2f(static_cast<float>(u.x), static_cast<float>(u.y));
            m_motion.time = Timeline(now, now + tickSeconds * 0.55);

            m_dir = DirectionUtil::fromDelta(dx, dy, old->m_dir);
        }
    }

    void setAssetId(modlib::AssetId assetId) {
        if (assetId != modlib::kInvalidAssetId) {
            m_assetId = assetId;
        }
    }
};

class VisualWorld {
    std::unordered_map<size_t, VisualUnit> m_units;
    std::vector<Corpse> m_corpses;

public:
    const std::unordered_map<size_t, VisualUnit> &entities() const { return m_units;   }
    const std::vector<Corpse>                  &corpses() const { return m_corpses; }

    void update(double) {
    }

    void applySnapshot(
        const WorldSnap &snap,
        double now,
        double tickSeconds,
        const std::vector<AssetChangeEvent> &assetChanges
    ) {
        std::unordered_set<size_t> present;

        for (size_t i = 0; i < snap.entities.size(); ++i) {
            const UnitSnap &u = snap.entities[i];

            if (u.hp <= 0) {
                addCorpseOnce(u.x, u.y, u.id);
                present.insert(u.id);
                continue;
            }

            present.insert(u.id);

            VisualUnit next;
            auto old = m_units.find(u.id);

            if (old == m_units.end()) {
                next.applySnapshot(u, NULL, now, tickSeconds);
            } else {
                next.applySnapshot(u, &old->second, now, tickSeconds);
            }

            m_units[u.id] = next;
        }

        eraseMissingUnits(present);
        applyAssetChanges(assetChanges);
    }

private:
    bool corpseExists(size_t id) const {
        for (size_t i = 0; i < m_corpses.size(); ++i) {
            if (m_corpses[i].id == id) return true;
        }

        return false;
    }

    void addCorpseOnce(int x, int y, size_t id) {
        if (!corpseExists(id)) {
            m_corpses.push_back(Corpse(x, y, id));
        }
    }

    void eraseMissingUnits(const std::unordered_set<size_t> &present) {
        std::vector<size_t> missing;

        for (const auto &unit : m_units) {
            if (present.find(unit.first) == present.end()) {
                missing.push_back(unit.first);
            }
        }

        for (size_t i = 0; i < missing.size(); ++i) {
            const size_t id = missing[i];
            auto it = m_units.find(id);

            if (it == m_units.end()) continue;

            addCorpseOnce(it->second.x(), it->second.y(), id);
            m_units.erase(it);
        }
    }

    void applyAssetChanges(const std::vector<AssetChangeEvent> &assetChanges) {
        for (const AssetChangeEvent &change : assetChanges) {
            auto it = m_units.find(change.entityID);
            if (it == m_units.end()) continue;

            it->second.setAssetId(change.assetId);
        }
    }
};

} // namespace vis
