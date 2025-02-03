// Copyright 2025-present mike.shevchenko@gmail.com. Licensed under www.mozilla.org/MPL/2.0/

#include "vampires.h"

#include <algorithm>
#include <cstdlib>

namespace ms::vampires_nx_vms_plugin {

std::string Vampires::toString(Direction direction)
{
    switch (direction)
    {
        case Direction::up: return "up";
        case Direction::upRight: return "upRight";
        case Direction::right: return "right";
        case Direction::downRight: return "downRight";
        case Direction::down: return "down";
        case Direction::downLeft: return "downLeft";
        case Direction::left: return "left";
        case Direction::upLeft: return "upLeft";
        default:
            NX_KIT_ASSERT(false);
            return "unknown";
    }
}

std::string Vampires::Item::toString(Kind kind)
{
    switch (kind)
    {
        case Kind::player: return "player";
        case Kind::wall: return "wall";
        case Kind::vampire: return "vampire";
        case Kind::border: return "border";
        default:
            NX_KIT_ASSERT(false);
            return "unknown";
    }
}

Vampires::Vampires(
    int width, int height, int vampireCount, int wallCount,
    std::shared_ptr<Item::Factory> itemFactory)
    :
    width(width),
    height(height),
    vampireCount(vampireCount),
    wallCount(wallCount),
    m_itemFactory(itemFactory)
{
    NX_KIT_ASSERT(width >= 7);
    NX_KIT_ASSERT(height >= 7);
    NX_KIT_ASSERT(vampireCount >= 1);
    NX_KIT_ASSERT(wallCount >= 1);
    NX_KIT_ASSERT(wallCount <= (width - 4) * (height - 4) - /* cell for player */ 1);

    NX_KIT_ASSERT(m_itemFactory);

    m_field.resize(height);
    for (auto& line: m_field)
        line.resize(width);

    initGame();
}

std::shared_ptr<Vampires::Item> Vampires::itemAt(int x, int y) const
{
    if (!NX_KIT_ASSERT(x >= 0) || !NX_KIT_ASSERT(x < width) ||
        !NX_KIT_ASSERT(y >= 0) || !NX_KIT_ASSERT(y < height))
    {
        return nullptr;
    }

    return m_field[y][x];
}

void Vampires::printField() const
{
    // Print two chars per cell to obtain a visually square field.

    std::string field = "VAMPIRES FIELD:\n";

    for (int y = 0; y < height; ++y)
    {
        std::string line;
        for (int x = 0; x < width; x++)
        {
            if (!m_field[y][x])
            {
                line += "  ";
            }
            else
            {
                switch (m_field[y][x]->kind)
                {
                    case Item::Kind::player: line += "}{"; break;
                    case Item::Kind::wall: line += "[]"; break;
                    case Item::Kind::vampire: line += "><"; break;
                    case Item::Kind::border: line += "()"; break;
                    default: NX_KIT_ASSERT(false);
                }
            }
        }
        field += line + "\n";
    }
    NX_PRINT << field;
}

/** NOTE: The field cell must be empty. */
std::shared_ptr<Vampires::Item> Vampires::createItem(Item::Kind kind, int x, int y)
{
    NX_KIT_ASSERT(!m_field[y][x]);

    const std::shared_ptr<Item> item(m_itemFactory->createItem(kind, x, y));
    m_field[y][x] = item;

    return item;
}

/** NOTE: The field cell must be empty. */
void Vampires::moveItem(std::shared_ptr<Item> item, int x, int y)
{
    NX_KIT_ASSERT(!m_field[y][x]);
    NX_KIT_ASSERT(m_field[item->y()][item->x()] == item); //< Check the field consistency.

    std::swap(m_field[item->y()][item->x()], m_field[y][x]);
    item->setX(x);
    item->setY(y);
}

bool Vampires::fieldHas(int x, int y, Item::Kind kind) const
{
    const auto& item = m_field[y][x];
    return item && item->kind == kind;
}

void Vampires::initGame()
{
    // Draw the border: the outermost circle of the field.
    for (int x = 0; x < width; x++)
    {
        createItem(Item::Kind::border, x, 0);
        createItem(Item::Kind::border, x, height - 1);
    }
    for (int y = 1; y < height - 1; ++y)
    {
        createItem(Item::Kind::border, 0, y);
        createItem(Item::Kind::border, width - 1, y);
    }

    // Settle the vampires along the inner circle of the border.
    int position = 0; //< The "integral" part of the next vampire coordinate.
    int accumulator = 0; //< The "fractional" part of the next vampire coordinate.
    const int maxVampireCount = 2 * (width - 2) + 2 * (height - 4);
    int step = maxVampireCount % vampireCount;
    int positionInc = maxVampireCount / vampireCount;
    for (int i = 0; i < vampireCount; ++i)
    {
        int x = -1;
        int y = -1;
        if (position <= width - 3) //< Top side.
        {
            x = position + 1;
            y = 1;
        }
        else if (position <= width + height - 6) //< Right side.
        {
            x = width - 2;
            y = position - width + 4;
        }
        else if (position <= 2 * width + height - 9) //< Bottom side.
        {
            x = 2 * width + height - 8 - position;
            y = height - 2;
        }
        else //< Left side.
        {
            x = 1;
            y = 2 * width + 2 * height - 11 - position;
        }

        position += positionInc;
        accumulator += step;
        if (accumulator >= vampireCount)
        {
            accumulator -= vampireCount;
            ++position;
        }

        m_vampires.push_back(Vampire{createItem(Item::Kind::vampire, x, y), 0});
    }
    NX_KIT_ASSERT(m_vampires.size() == vampireCount);

    // Settle the player at the center.
    m_player = createItem(Item::Kind::player, width / 2, height / 2);

    // Put the walls randomly.
    struct Wall
    {
        Wall(int x, int y): x(x), y(y) {}
        int x = -1;
        int y = -1;
    };
    std::vector<Wall> walls;
    for (int y = 2; y < height - 2; ++y)
    {
        for (int x = 2; x < width - 2; ++x)
        {
            if (!m_field[y][x])
                walls.emplace_back(x, y);
        }
    }

    int spacesLeft = (int) walls.size();
    for (int wallsLeft = wallCount; wallsLeft != 0; --wallsLeft)
    {
        int i = (2 * rand() * (spacesLeft - 1) + RAND_MAX) / (2 * RAND_MAX);
        createItem(Item::Kind::wall, walls[i].x, walls[i].y);
        if (i != spacesLeft - 1)
            walls.erase(walls.begin() + i);
        --spacesLeft;
    }
}

struct Distance
{
    int x = 0;
    int y = 0;
};

static Distance directionToDistance(Vampires::Direction direction)
{
    Distance d;
    switch (direction)
    {
        case Vampires::Direction::up: --d.y; break;
        case Vampires::Direction::upRight: ++d.x; --d.y; break;
        case Vampires::Direction::right: ++d.x; break;
        case Vampires::Direction::downRight: ++d.x; ++d.y; break;
        case Vampires::Direction::down: ++d.y; break;
        case Vampires::Direction::downLeft: --d.x; ++d.y; break;
        case Vampires::Direction::left: --d.x; break;
        case Vampires::Direction::upLeft: --d.x; --d.y; break;
        default:
            NX_KIT_ASSERT(false);
    }
    return d;
}

Vampires::PlayerResult Vampires::movePlayer(Vampires::Direction direction)
{
    const Distance d = directionToDistance(direction);

    const int newX = m_player->x() + d.x;
    const int newY = m_player->y() + d.y;
    if (fieldHas(newX, newY, Item::Kind::vampire))
        return PlayerResult::lost;

    // Find the cell which should be occupied. The loop is safe because of the border.
    int emptyX = newX;
    int emptyY = newY;
    while (fieldHas(emptyX, emptyY, Item::Kind::wall))
    {
        emptyX += d.x;
        emptyY += d.y;
    }

    if (m_field[emptyY][emptyX]) //< Unable to move: the cell after all walls (if any) is non-empty.
        return PlayerResult::ok;

    // Push the walls if needed, starting with the last one in the row.
    while (!(emptyX == newX && emptyY == newY))
    {
        const int wallX = emptyX - d.x;
        const int wallY = emptyY - d.y;
        moveItem(m_field[wallY][wallX], emptyX, emptyY);
        emptyX = wallX;
        emptyY = wallY;
    }

    moveItem(m_player, newX, newY);
    return PlayerResult::ok;
}

Vampires::VampireResult Vampires::moveVampires()
{
    // Calculate the distance to the player for each Vampire.
    for (auto& vampire: m_vampires)
    {
        const int dx = vampire.item->x() - m_player->x();
        const int dy = vampire.item->y() - m_player->y();
        vampire.d = dx * dx + dy * dy;
    }

    // Sort Vampires by the distance to the player, the closest first.
    std::sort(m_vampires.begin(), m_vampires.end(),
        [](const Vampire& v1, const Vampire& v2)
        {
            return v1.d > v2.d;
        });

    // Each vampire moves to come closer to the player, and if there is any move, it must move.
    bool hasSomeVampiresMoved = false;
    for (const auto& vampire: m_vampires)
    {
        int minDd = INT_MAX;
        Distance minDistance{};
        for (int dir = 0; dir < (int) Direction::count; ++dir)
        {
            const Distance d = directionToDistance((Direction) dir);

            if (fieldHas(vampire.item->x() + d.x, vampire.item->y() + d.y, Item::Kind::player))
                return VampireResult::lost;

            if (m_field[vampire.item->y() + d.y][vampire.item->x() + d.x])
                continue; //< The intended move is impossible: the cell is occupied.

            const int cx = 2 * (vampire.item->x() - m_player->x());
            const int cy = 2 * (vampire.item->y() - m_player->y());
            const int dd = ((d.x != 0)
                ? ((d.x == 1) ? (1 + cx) : (1 - cx))
                : 0)
                +
                ((d.y != 0)
                ? ((d.y == 1) ? (1 + cy) : (1 - cy))
                : 0);
            if (minDd > dd)
            {
                minDd = dd;
                minDistance = d;
            }
        }
        if (minDd == INT_MAX) //< There is no move for this Vampire: skip it.
            continue;

        moveItem(
            vampire.item, vampire.item->x() + minDistance.x, vampire.item->y() + minDistance.y);
        hasSomeVampiresMoved = true;
    }
    return hasSomeVampiresMoved ? VampireResult::ok : VampireResult::win;
}

} // namespace ms::vampires_nx_vms_plugin
