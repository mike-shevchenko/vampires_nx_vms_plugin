// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#pragma once

#include <memory>
#include <string>

#include <nx/kit/debug.h>

namespace ms::vampires_nx_vms_plugin {

class Vampires
{
public:
    enum class Direction
    {
        up,
        upRight,
        right,
        downRight,
        down,
        downLeft,
        left,
        upLeft,
        count
    };

    static std::string toString(Direction direction);

    enum class PlayerResult
    {
        ok,
        lost,
    };

    enum class VampireResult
    {
        ok,
        lost,
        win,
    };

    /** Inherit to use custom Item objects. */
    class Item
    {
    public:
        enum class Kind
        {
            player,
            wall,
            vampire,
            border,
        };

        static std::string toString(Kind kind);

        /** Inherit to provide a factory for custom Item objects. */
        class Factory
        {
        public:
            virtual ~Factory() = default;

            virtual Item* createItem(Kind kind, int x, int y) const
            {
                return new Item(kind, x, y);
            }
        };

        virtual std::string toString() const
        {
            return nx::kit::utils::format("%s(%d, %d)", toString(kind), m_x, m_y);
        }

    public:
        Item(Kind kind, int x, int y): kind(kind), m_x(x), m_y(y) {}
        virtual ~Item() = default;
        int x() const { return m_x; }
        void setX(int x) { m_x = x; }
        int y() const { return m_y; }
        void setY(int y) { m_y = y; }

    public:
        const Kind kind;

    private:
        int m_x = -1;
        int m_y = -1;
    };

public:
    Vampires(
        int width, int height, int vampireCount, int wallCount,
        std::shared_ptr<Item::Factory> itemFactory = std::make_shared<Item::Factory>());

    PlayerResult movePlayer(Direction direction);

    VampireResult moveVampires();

public:
    const int width = -1;
    const int height = -1;
    const int vampireCount = -1;
    const int wallCount = -1;

    std::shared_ptr<Item> itemAt(int x, int y) const;

    /** Intended for debug. */
    void printField() const;

private:
    std::shared_ptr<Item> createItem(Item::Kind kind, int x, int y);
    void moveItem(std::shared_ptr<Item> item, int x, int y);
    bool fieldHas(int x, int y, Item::Kind kind) const;
    void initGame();

private:
    const std::shared_ptr<Item::Factory> m_itemFactory;

    std::vector<std::vector<std::shared_ptr<Item>>> m_field; /**< Null item means an empty cell. */

    struct Vampire
    {
        std::shared_ptr<Item> item;
        int d = -1; /**< Distance to the player. */

        Vampire(std::shared_ptr<Item> item, int d): item(item), d(d) {}
    };

    std::vector<Vampire> m_vampires;

    std::shared_ptr<Item> m_player;
};

} // namespace ms::vampires_nx_vms_plugin
