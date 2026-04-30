#pragma once

class Entity
{
public:
    using ID = uint64_t;
    using Type = bmsg::Char64;

    Entity (Type type, Tile* tile);

    ~Entity ();


    ID    getID   () const;
    Type  getType () const;
    
    Tile*    getTile     () const;                                                                                  // Please refrain from using this method for safety reasons
    Vec2D<>  getPosition () const;
    void     setTile     (Tile*   tile);
    void     setPosition (Vec2D<> position);

    Event<Vec2D<>> EvEntityMoved;
};
