
#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || \
    defined(__MINGW32__) || defined(__BORLANDC__)
#define OS_WIN
#endif

// M_PI not defined on windows?
#ifdef OS_WIN
#define _USE_MATH_DEFINES
#include <cmath>
#endif

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Mouse.hpp>
#include <iostream>
#include <memory>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_map>

#define FMT_HEADER_ONLY
#include "fmt/format.h"
#include "util.h"

struct Timer {
    sf::Time totalTime;
    sf::Time timeLeft;

    Timer(sf::Time t) : totalTime(t), timeLeft(t) {}

    void resetTimer() { timeLeft = totalTime; }
    bool isTime() { return timeLeft <= sf::Time::Zero; }
    void passTime(sf::Time dt) { timeLeft -= dt; }
};

enum MenuState {
    EXIT = -1,

    ROOT = 0,
    ABOUT,
    PAUSED,
    SETTINGS,
    GAME,
};

// size of each grid spot
constexpr float GRID_SIZEF = 100.f;
constexpr int GRID_SIZE = static_cast<int>(GRID_SIZEF);
// number of grid items in the world
constexpr int WORLD_GRID_SIZE = 10;
// TODO - can we lower this? it feels too long
constexpr float PLAYER_TILE_REACH = 0.5;
constexpr float P_MOVE_DIST = 0.05;

struct Tile : public sf::Drawable {
    sf::RectangleShape shape;

    Tile() {
        shape.setSize(vec2f{GRID_SIZEF, GRID_SIZEF});
        shape.setFillColor(sf::Color::White);
        shape.setOutlineThickness(1.0f);
        shape.setOutlineColor(sf::Color::Black);
    }
    void setPosition(vec2f position) { shape.setPosition(position); }
    vec2f getPosition() { return shape.getPosition(); }

    void update(sf::Time dt) {}

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        target.draw(shape);
    }
};
std::vector<std::vector<Tile>> tilemap;

/// /// /// /// /// /// /// /// /// /// /// /// /// /// /// ///
struct Item : public sf::Drawable {
    std::string name;
    double price;
    vec2f tilePosition;
    sf::Color color;

    Item(std::string n, double p) : name(n), price(p) {}
    Item(std::string n, double p, sf::Color c) : name(n), price(p), color(c) {}

    ~Item() {}

    Item(const Item &i)
        : name(i.name),
          price(i.price),
          tilePosition(i.tilePosition),
          color(i.color) {}
    Item &operator=(const Item &i) {
        if (this == &i) return *this;
        this->name = i.name;
        this->price = i.price;
        this->tilePosition = i.tilePosition;
        this->color = i.color;
        return *this;
    }

    bool operator<(const Item &i) const { return this->name < i.name; }

    void update(sf::Time dt) {}

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        sf::RectangleShape s =
            sf::RectangleShape(vec2f{GRID_SIZEF / 4, GRID_SIZEF / 4});
        s.setPosition(
            vec2f{tilePosition.x * GRID_SIZEF, tilePosition.y * GRID_SIZEF});
        s.setFillColor(color);
        target.draw(s);
    }
};

const std::vector<const Item> all_items = std::vector<const Item>{
    Item("apple", 1.0, sf::Color(196, 51, 64)),
    Item("potato", 1.0, sf::Color(182, 158, 135)),
};

struct Desire {
    Item item;
    int amount;
    Desire(Item i, int a) : item(i), amount(a) {}
    ~Desire() {}
    Desire(const Desire &d) : item(d.item), amount(d.amount) {}

    Desire &operator=(const Desire &d) {
        if (this == &d) return *this;
        this->item = d.item;
        this->amount = d.amount;
        return *this;
    }
};
typedef std::vector<Desire> Desires;

struct Shelf : public sf::Drawable {
    Desires contents;
    vec2f tilePosition;

    sf::RectangleShape shape;

    Shelf() {
        shape = sf::RectangleShape(vec2f{GRID_SIZEF, GRID_SIZEF});
        shape.setFillColor(sf::Color(150, 150, 150));
    }

    Shelf(Item i) {
        shape = sf::RectangleShape(vec2f{GRID_SIZEF, GRID_SIZEF});
        shape.setFillColor(sf::Color(150, 150, 150));
        contents.push_back(Desire(i, 1));
    }

    ~Shelf() {}
    Shelf(const Shelf &s)
        : contents(s.contents), tilePosition(s.tilePosition), shape(s.shape) {}
    Shelf &operator=(const Shelf &s) {
        this->contents = s.contents;
        this->tilePosition = s.tilePosition;
        this->shape = s.shape;
        return *this;
    }

    void setTile(vec2f tile_pos) {
        tilePosition = tile_pos;
        shape.setPosition(
            vec2f{tilePosition.x * GRID_SIZEF, tilePosition.y * GRID_SIZEF});
    }

    auto hasItem(const Item &want) const {
        return std::find_if(
            contents.begin(), contents.end(),
            [&want](const Desire &d) { return d.item.name == want.name; });
    }

    void update(sf::Time elapsed) {
        // Desire is empty
        auto is_empty =
            std::remove_if(contents.begin(), contents.end(),
                           [](const Desire &d) { return d.amount == 0; });
        contents.erase(is_empty, contents.end());
        //

        for (Desire d : contents) {
            d.item.update(elapsed);
        }
    }

    void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        target.draw(shape);
        for (Desire d : contents) {
            d.item.tilePosition = tilePosition;
            target.draw(d.item);
        }
    }
};
std::vector<std::unique_ptr<Shelf>> shelves;

bool inBounds(vec2i pos) {
    return (pos.x <= WORLD_GRID_SIZE) && (pos.x >= 0) &&
           (pos.y < WORLD_GRID_SIZE) && (pos.y >= 0);
}

bool isSame(vec2i s, vec2i g) { return s.x == g.x && s.y == g.y; }
bool isSame(vec2f s, vec2f g) {
    return (abs(g.x - s.x) < 0.05 && abs(g.y - s.y) < 0.05);
}

bool isCloseEnough(vec2i s, vec2i g, int d) {
    return (abs(g.x - s.x) <= d && abs(g.y - s.y) <= d);
}

bool isCloseEnough(vec2f s, vec2f g, float d) {
    return (abs(g.x - s.x) <= d && abs(g.y - s.y) <= d);
}

bool isValidLocation(vec2i pos) {
    if (!inBounds(pos)) {
        return false;
    }
    for (const std::unique_ptr<Shelf> &s : shelves) {
        if (s->tilePosition.x == pos.x && s->tilePosition.y == pos.y) {
            return false;
        }
    }
    return true;
}

struct VectorHash {
    std::size_t operator()(sf::Vector2<int> a) const {
        return std::hash<int>()(a.x) ^ ((std::hash<int>()(a.y) << 1) >> 1);
    }
};

struct QueueItem {
    vec2i location;
    double score;
    QueueItem(vec2i l, double s) : location(l), score(s) {}
    bool operator<(const QueueItem &s) const { return this->score > s.score; }
};

struct Score {
    double value;
    Score() : value(WORLD_GRID_SIZE * WORLD_GRID_SIZE) {}
    Score(double s) : value(s) {}
    bool operator<(const Score &s) const { return this->value < s.value; }
};

std::vector<vec2i> reconstruct_path(
    const std::unordered_map<vec2i, vec2i, VectorHash> &cameFrom, vec2i cur) {
    std::vector<vec2i> path;
    path.push_back(cur);
    while (cameFrom.find(cur) != cameFrom.end()) {
        cur = cameFrom.at(cur);
        path.push_back(cur);
    }
    return path;
}

std::vector<vec2i> astar(const vec2i &start, const vec2i &g) {
    auto heuristic = [](const vec2i &s, const vec2i &g) {
        return abs(s.x - g.x) + abs(s.y - g.y);
    };

    vec2i goal = g;

    // if (!isValidLocation(goal)) {
    // // if the goal isnt valid, theres no way for us to reconstruct_path
    // // so lets see if any of the neighbors are valid and use one of those
    // // people can reach one tile so its not the biggest deal.
    // std::priority_queue<QueueItem> closest_n;
    // const int x[] = {0, 0, 1, -1, -1, 1, -1, 1};
    // const int y[] = {1, -1, 0, 0, -1, -1, 1, 1};
    // for (int i = 0; i < 8; i++) {
    // vec2i neighbor = vec2i{g.x + x[i], g.y + y[i]};
    // if (!isValidLocation(neighbor)) {
    // continue;
    // }
    // closest_n.push(QueueItem(neighbor, heuristic(neighbor, start)));
    // }
    // if (closest_n.empty()) {
    // // TODO Should we check all the neighbors neighbors?
    // return std::vector<vec2i>{};
    // }
    // goal = closest_n.top().location;
    // }

    std::priority_queue<QueueItem> openSet;
    openSet.push(QueueItem{start, 0});

    std::unordered_map<vec2i, vec2i, VectorHash> cameFrom;
    std::unordered_map<vec2i, double, VectorHash> distTraveled;

    distTraveled[start] = 0;

    while (!openSet.empty()) {
        QueueItem qi = openSet.top();
        openSet.pop();
        auto cur = qi.location;
        if (isSame(cur, goal)) break;

        const int x[] = {0, 0, 1, -1, -1, 1, -1, 1};
        const int y[] = {1, -1, 0, 0, -1, -1, 1, 1};
        for (int i = 0; i < 8; i++) {
            vec2i neighbor = vec2i{cur.x + x[i], cur.y + y[i]};
            if (!isValidLocation(neighbor) || isSame(neighbor, goal)) {
                continue;
            }
            double newCost = distTraveled[cur] + dist_i(cur, neighbor);
            if (distTraveled.find(neighbor) == distTraveled.end() ||
                newCost < distTraveled[neighbor]) {
                distTraveled[neighbor] = newCost;
                double prio = newCost + heuristic(neighbor, goal);
                openSet.push(QueueItem{neighbor, prio});
                cameFrom[neighbor] = cur;
            }
        }
    }
    return reconstruct_path(cameFrom, goal);
}

std::vector<std::unique_ptr<Shelf>>::iterator shelf_with_item(
    const Item &want) {
    std::vector<std::unique_ptr<Shelf>>::iterator it = std::find_if(
        shelves.begin(), shelves.end(),
        [&want](const std::unique_ptr<Shelf> &s) {
            auto itt = std::find_if(
                s->contents.begin(), s->contents.end(),
                [&want](const Desire &d) { return d.item.name == want.name; });
            return itt != s->contents.end();
        });
    return it;
}

std::vector<std::unique_ptr<Shelf>>::iterator closest_shelf_to_current_pos(
    vec2f pos) {
    int min_d = -1;
    std::vector<std::unique_ptr<Shelf>>::iterator s = shelves.end();
    for (auto it = shelves.begin(); it != shelves.end(); it++) {
        auto d = dist((*it)->tilePosition, pos);
        if (min_d == -1 || d < min_d) {
            min_d = d;
            s = it;
        }
    }
    return s;
}

const std::vector<sf::Color> PCOLORS = std::vector<sf::Color>{
    sf::Color(46, 20, 0),  sf::Color(66, 28, 0),     sf::Color(86, 37, 0),
    sf::Color(164, 70, 0), sf::Color(255, 229, 180),
};

struct Person : public sf::Drawable {
    const std::vector<sf::Color> PCOLORS = std::vector<sf::Color>{
        sf::Color(46, 20, 0),  sf::Color(66, 28, 0),     sf::Color(86, 37, 0),
        sf::Color(164, 70, 0), sf::Color(255, 229, 180),
    };

    sf::RectangleShape shape;
    vec2f tilePosition;

    Person() {
        random_selector<> rs{};
        shape = sf::RectangleShape(vec2f{GRID_SIZEF / 2, GRID_SIZEF / 2});
        shape.setFillColor(rs(PCOLORS));
    }

    void setFillColor(sf::Color c) { shape.setFillColor(c); }

    vec2f move_toward_target(vec2f pos, vec2f t, float px) {
        double xdiff = t.x - pos.x;
        double ydiff = t.y - pos.y;
        if (xdiff < -px)
            pos.x -= px;
        else if (xdiff > px)
            pos.x += px;
        else if (ydiff < -px)
            pos.y -= px;
        else if (ydiff > px)
            pos.y += px;
        return pos;
    }

    void setTile(vec2f tile_pos) {
        tilePosition = tile_pos;
        shape.setPosition(
            vec2f{tilePosition.x * GRID_SIZEF, tilePosition.y * GRID_SIZEF});
    }

    void update(sf::Time dt) {}

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        target.draw(shape);
    }
};

struct PersonWithDesire : public Person {
    Desires desires;

    int desireIndex = -1;
    vec2f desireLocation;
    std::vector<vec2i> followPath;

    void add_desire(const Desire &d) { desires.push_back(d); }

    void clean_up_desires_fulfilled() {
        desires.erase(
            std::remove_if(desires.begin(), desires.end(),
                           [](const Desire &d) { return d.amount == 0; }),
            desires.end());
    }

    virtual vec2f location_of_desire() = 0;
    virtual void run_shelf_action(vec2f pos) = 0;

    void head_to_checkout_if_done_shopping() {
        if (desires.size() != 0) return;
        // TODO go to checkout when done
        tilePosition =
            move_toward_target(tilePosition, vec2f{0.f, 0.f}, P_MOVE_DIST);
        setTile(tilePosition);
    }

    void select_a_desire() {
        if (desires.size() == 0) return;
        if (desireIndex != -1) return;
        // we have no selected desire
        desireLocation = location_of_desire();
    }

    void find_a_way_there() {
        if (desires.size() == 0) return;
        if (!followPath.empty()) return;
        followPath = astar(vtoi(tilePosition), vtoi(desireLocation));
        std::cout << "Generated a new path to my desired location "
                  << desireLocation << std::endl;
        std::cout << followPath << std::endl;
    }

    void follow_your_heart() {
        if (desires.size() == 0) return;
        if (followPath.empty()) return;
        auto local_target = vtof(followPath.back());
        tilePosition =
            move_toward_target(tilePosition, local_target, P_MOVE_DIST);
        setTile(tilePosition);
        if (isSame(tilePosition, local_target)) followPath.pop_back();
    }

    void meet_your_soul_mate() {
        if (desires.size() == 0) return;
        bool reachedShelf =
            isCloseEnough(tilePosition, desireLocation, PLAYER_TILE_REACH);
        if (!reachedShelf) return;
        run_shelf_action(tilePosition);
    }

    void update(sf::Time dt) {
        Person::update(dt);

        head_to_checkout_if_done_shopping();
        clean_up_desires_fulfilled();
        meet_your_soul_mate();
        follow_your_heart();
        find_a_way_there();
        select_a_desire();
    }
};

struct StockClerk : public PersonWithDesire {
    StockClerk() {
        setTile(randVecf(0, WORLD_GRID_SIZE));

        Desire d(all_items[0], 10);
        add_desire(d);
        setFillColor(sf::Color::Blue);
    }

    void run_shelf_action(vec2f pos) {
        if (shelves.empty()) return;
        /*
         * As the stock clerk, lets find the close shelf
         * and add our item to it :)
         * */

        auto shelf_it = closest_shelf_to_current_pos(pos);
        // Does this shelf already have this item?
        Desire des = desires[desireIndex];
        const std::unique_ptr<Shelf> &shelf = *shelf_it;

        bool found = false;
        for (int i = 0; i < shelf->contents.size(); i++) {
            if (shelf->contents[i].item.name == des.item.name) {
                shelf->contents[i].amount += 1;
                found = true;
            }
        }

        if (!found) {
            shelf->contents.push_back(Desire(des.item, 1));
        }

        des.amount -= 1;

        // maybe we want to have a max stack size or something
        // and not add everything
        // write back so it can be cleaned up
        desires[desireIndex] = des;
        desireIndex = -1;
    }

    vec2f location_of_desire() {
        // find any empty shelves first
        std::vector<std::unique_ptr<Shelf>>::iterator it =
            std::find_if(shelves.begin(), shelves.end(),
                         [](const std::unique_ptr<Shelf> &s) {
                             return s->contents.empty();
                         });
        if (it != shelves.end()) {
            desireIndex = 0;
            return (*it)->tilePosition;
        }

        // TODO - wander?
        // just send them to the top left and wait
        return vec2f{0, 0};
    }
};

struct Customer : public PersonWithDesire {
    Customer() {
        vec2f start;
        do {
            start = randVecf(0, WORLD_GRID_SIZE);
        } while (!isValidLocation(vtoi(start)));
        setTile(start);

        Desire d(all_items[0], 1);
        add_desire(d);
    }
    Customer(vec2f pos) {
        setTile(pos);
        Desire d(all_items[0], 1);
        add_desire(d);
    }

    vec2f location_of_desire() {
        for (int i = 0; i < desires.size(); i++) {
            Item item = desires[i].item;
            auto shelf_it = shelf_with_item(item);
            if (shelf_it != shelves.end()) {
                desireIndex = i;
                return (*shelf_it)->tilePosition;
            } else {
                log(fmt::format("no shelf with what i want {}", item.name),
                    true);
            }
        }
        // TODO replace with pain or wander
        // didnt find any shelves, just go somewhere for now
        vec2f wander;
        do {
            wander = randVecf(0, WORLD_GRID_SIZE);
        } while (!isValidLocation(vtoi(wander)));
        return wander;
    }

    /*
     * As the customer, now that we are near our goal shelf,
     * find the one we are trying to reach and remove the items from it
     * and put it in our pocket
     * */
    void run_shelf_action(vec2f pos) {
        auto matching_shelf = shelves.end();
        auto matching_desire = shelves[0]->contents.end();
        for (auto it = shelves.begin(); it != shelves.end(); it++) {
            if (dist(pos, (*it)->tilePosition) < PLAYER_TILE_REACH) continue;

            auto itt = std::find_if(
                (*it)->contents.begin(), (*it)->contents.end(),
                [&](const Desire &d) {
                    return d.item.name == desires[desireIndex].item.name;
                });
            if (itt == (*it)->contents.end()) {
                continue;
            }
            matching_shelf = it;
            matching_desire = itt;
        }
        if (matching_shelf == shelves.end()) {
            // log("no shelf within reach has our item");
            // welp lets give up,
            desireIndex = -1;
            return;
        }

        // nice we found a matching shelf

        // do they have enough?
        if (matching_desire->amount >= desires[desireIndex].amount) {
            matching_desire->amount -= desires[desireIndex].amount;
            desires[desireIndex].amount = 0;
        } else {
            // they dont have enough....
            desires[desireIndex].amount -= matching_desire->amount;
            // take everything
            matching_desire->amount = 0;
        }
        desireIndex = -1;
    }
};

struct World : public sf::Drawable {
    vec2i mousePosWindow;
    vec2f mousePosView;
    vec2i mousePosGrid;
    sf::RectangleShape gridOutline;

    std::vector<Customer *> customers;
    std::vector<PersonWithDesire *> employees;
    Desires globalStock;
    Desires globalForPurchase;
    Desires globalWants;
    int numIdle;

    World() {
        tilemap.resize(WORLD_GRID_SIZE, std::vector<Tile>());
        for (int i = 0; i < WORLD_GRID_SIZE; i++) {
            tilemap[i].resize(WORLD_GRID_SIZE, Tile());
            for (int j = 0; j < WORLD_GRID_SIZE; j++) {
                tilemap[i][j].setPosition(
                    vec2f{i * GRID_SIZEF, j * GRID_SIZEF});
            }
        }

        gridOutline = sf::RectangleShape(vec2f{GRID_SIZE, GRID_SIZE});
        gridOutline.setFillColor(sf::Color::Transparent);
        gridOutline.setOutlineThickness(1.0f);
        gridOutline.setOutlineColor(sf::Color::Green);

        // for (int i = 0; i < 1; i++) {
        // customers.push_back(new Customer());
        // }
        //
        // for (int i = 0; i < 10; i++) {
        // employees.push_back(new StockClerk());
        // }

        for (int i = 0; i < WORLD_GRID_SIZE - 3; i++) {
            for (int j = 0; j < WORLD_GRID_SIZE - 2; j += 2) {
                std::unique_ptr<Shelf> s(new Shelf());
                s->setTile(vtof(vec2i{1 + j, i}));
                shelves.push_back(std::move(s));
            }
        }

        std::unique_ptr<Shelf> s(new Shelf(all_items[0]));
        s->setTile(vec2f{WORLD_GRID_SIZE - 2, WORLD_GRID_SIZE - 2});
        shelves.push_back(std::move(s));

        numIdle = 0;
    }

    ~World() {
        for (int i = customers.size() - 1; i >= 0; i--) {
            delete customers[i];
        }

        for (int i = employees.size() - 1; i >= 0; i--) {
            delete employees[i];
        }
    }

    void update(sf::Time dt) {
        gridOutline.setPosition(mousePosGrid.x * GRID_SIZE,
                                mousePosGrid.y * GRID_SIZE);
        globalStock.clear();
        globalForPurchase.clear();
        globalWants.clear();
        numIdle = 0;

        // What supplies do we have (including in the back)
        for (int i = 0; i < employees.size(); i++) {
            employees[i]->update(dt);

            for (const Desire &d : employees[i]->desires) {
                auto it = std::find_if(globalStock.begin(), globalStock.end(),
                                       [&](const Desire &des) {
                                           return d.item.name == des.item.name;
                                       });
                if (it == globalStock.end()) {
                    globalStock.push_back(d);
                } else {
                    it->amount += d.amount;
                }
            }
        }

        // Whats out on shelves
        for (int i = 0; i < shelves.size(); i++) {
            shelves[i]->update(dt);

            for (const Desire &d : shelves[i]->contents) {
                auto it = std::find_if(globalForPurchase.begin(),
                                       globalForPurchase.end(),
                                       [&](const Desire &des) {
                                           return d.item.name == des.item.name;
                                       });
                if (it == globalForPurchase.end()) {
                    globalForPurchase.push_back(d);
                } else {
                    it->amount = it->amount + d.amount;
                }
            }
        }

        for (int i = 0; i < customers.size(); i++) {
            customers[i]->update(dt);

            if (customers[i]->desires.size() == 0) {
                numIdle++;
            }

            for (const Desire &d : customers[i]->desires) {
                auto it = std::find_if(globalWants.begin(), globalWants.end(),
                                       [&](const Desire &des) {
                                           return d.item.name == des.item.name;
                                       });
                if (it == globalWants.end()) {
                    globalWants.push_back(d);
                } else {
                    it->amount += d.amount;
                }
            }
        }
    }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        int sx = 14;
        int sy = 12;

        int colS = (game_view->getCenter().x / GRID_SIZEF) - sx;
        int colE = (game_view->getCenter().x / GRID_SIZEF) + sx;
        int rowS = (game_view->getCenter().y / GRID_SIZEF) - sy;
        int rowE = (game_view->getCenter().y / GRID_SIZEF) + sy;

        if (colS < 0) colS = 0;
        if (colS > WORLD_GRID_SIZE) colS = WORLD_GRID_SIZE - 1;
        if (rowS < 0) rowS = 0;
        if (rowS > WORLD_GRID_SIZE) rowS = WORLD_GRID_SIZE - 1;

        if (colE < 0) colE = 0;
        if (colE > WORLD_GRID_SIZE) colE = WORLD_GRID_SIZE - 1;
        if (rowE < 0) rowE = 0;
        if (rowE > WORLD_GRID_SIZE) rowE = WORLD_GRID_SIZE - 1;

        for (int i = colS; i < colE; i++) {
            for (int j = rowS; j < rowE; j++) {
                target.draw(tilemap[i][j]);
            }
        }

        // TODO this doesnt work right now :(
        target.draw(gridOutline);

        for (int i = 0; i < shelves.size(); i++) {
            target.draw(*shelves[i]);
        }

        for (int i = 0; i < customers.size(); i++) {
            target.draw(*customers[i]);
        }

        for (int i = 0; i < employees.size(); i++) {
            target.draw(*employees[i]);
        }
    }

    void processEvent(sf::RenderWindow &app, sf::Event event) {
        mousePosWindow = sf::Mouse::getPosition(app);
        mousePosView = app.mapPixelToCoords(mousePosWindow);
        mousePosGrid = vec2i{
            (int)mousePosView.x / GRID_SIZE,
            (int)mousePosView.y / GRID_SIZE,
        };

        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == KEYS["1"])
                customers.push_back(new Customer());
            if (event.key.code == KEYS["2"])
                employees.push_back(new StockClerk());
        }
    }
};

struct UIText : public sf::Drawable {
    sf::Text content;

    UIText(float x, float y, std::string t = "", int size = 24,
           sf::Color color = sf::Color::Red,
           sf::Text::Style style = sf::Text::Regular) {
        content.setFont(*getGlobalFont());
        content.setString(t);
        content.setCharacterSize(size);
        content.setFillColor(color);
        content.setStyle(style);
        content.setPosition(vec2f{x, y});
    }

    void update(sf::Time dt) {}

    void setContentString(std::string update) { content.setString(update); }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        target.draw(content);
    }
};

constexpr int H_PADDING = (WIDTH / 50);
constexpr int V_PADDING = (HEIGHT / 50);
constexpr int V_SPACING = (HEIGHT / 45);

struct HUD : public sf::Drawable {
    World *world;

    typedef std::pair<std::string, UIText *> strUI;
    std::map<std::string, UIText *> uitext;

    HUD() {}

    void setWorld(World *w) { world = w; }

    void update(sf::Time dt) {
        if (world == nullptr) {
            return;
        }
        uitext.clear();

        uitext.insert(strUI("idle_shoppers",
                            new UIText(H_PADDING, V_PADDING + V_SPACING)));
        uitext["idle_shoppers"]->setContentString(
            fmt::format("Idle Shoppers: {}", world->numIdle));

        if (!world->globalStock.empty()) {
            uitext.insert(strUI(
                "stock",
                new UIText(H_PADDING,
                           V_PADDING + (V_SPACING * (uitext.size() + 1)))));
            uitext["stock"]->setContentString("Available Stock: ");
            for (const Desire &d : world->globalStock) {
                uitext.insert(strUI(
                    "supply" + d.item.name,
                    new UIText(H_PADDING,
                               V_PADDING + (V_SPACING * (uitext.size() + 1)))));
                uitext["supply" + d.item.name]->setContentString(
                    fmt::format("{} ({})", d.item.name, d.amount));
            }
        }

        if (!world->globalForPurchase.empty()) {
            uitext.insert(strUI(
                "shelved",
                new UIText(H_PADDING,
                           V_PADDING + (V_SPACING * (uitext.size() + 1)))));
            uitext["shelved"]->setContentString("Out on Shelves: ");

            for (const Desire &d : world->globalForPurchase) {
                uitext.insert(strUI(
                    "shelved" + d.item.name,
                    new UIText(H_PADDING,
                               V_PADDING + (V_SPACING * (uitext.size() + 1)))));
                uitext["shelved" + d.item.name]->setContentString(
                    fmt::format("{} ({})", d.item.name, d.amount));
            }
        }

        if (!world->globalWants.empty()) {
            uitext.insert(strUI(
                "wanted_header",
                new UIText(H_PADDING,
                           V_PADDING + (V_SPACING * (uitext.size() + 1)))));
            uitext["wanted_header"]->setContentString("Wants:");

            for (const Desire &d : world->globalWants) {
                uitext.insert(strUI(
                    d.item.name,
                    new UIText(H_PADDING,
                               V_PADDING + (V_SPACING * (uitext.size() + 1)))));
                uitext[d.item.name]->setContentString(
                    fmt::format("{} ({})", d.item.name, d.amount));
            }
        }
    }

    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const {
        for (strUI text : uitext) {
            target.draw(*text.second);
        }
    }

    void processEvent(sf::RenderWindow &app, sf::Event event) {
        if (event.type == sf::Event::KeyPressed) {
            switch (event.key.code) {
                case sf::Keyboard::Space:
                    break;
                default:
                    break;
            }
        }
    }
};

struct GameScreen {
    World *world;
    sf::View view;
    HUD hud;
    int zoom_level;
    float zoom_speed = 0.01;
    int min_zoom = -100;
    int max_zoom;

    GameScreen() {
        max_zoom = DEBUG ? 1000 : 50;
        world = new World();
        setGlobalWorld(world);
        hud.setWorld(world);

        view.reset(sf::FloatRect(0, 0, WIDTH, HEIGHT));
        zoom_level = 1.0f;
        game_view = &view;
    }

    ~GameScreen() { delete world; }

    void process_subscreen_events(sf::RenderWindow &app, sf::Event event) {
        world->processEvent(app, event);
        hud.processEvent(app, event);
    }

    void update_subscreens(sf::Time elapsed) {
        world->update(elapsed);
        hud.update(elapsed);
    }

    void handle_input(const double &dt) {
        double viewSpeed = zoom_level > 0 ? 1500.f : 500.f;
        double xd = 0.f, yd = 0.f;

        if (sf::Keyboard::isKeyPressed(KEYS["Left"])) xd = -viewSpeed * dt;
        if (sf::Keyboard::isKeyPressed(KEYS["Right"])) xd = viewSpeed * dt;
        if (sf::Keyboard::isKeyPressed(KEYS["Up"])) yd = -viewSpeed * dt;
        if (sf::Keyboard::isKeyPressed(KEYS["Down"])) yd = viewSpeed * dt;

        if (sf::Keyboard::isKeyPressed(KEYS["Scroll-In"]) &&
            zoom_level < max_zoom) {
            view.zoom(1 + zoom_speed);
            zoom_level += 1;
        }
        if (sf::Keyboard::isKeyPressed(KEYS["Scroll-Out"]) &&
            zoom_level > min_zoom) {
            view.zoom(1 - zoom_speed);
            zoom_level -= 1;
        }

        view.move(xd, yd);
    }

    virtual int run(sf::RenderWindow &app) {
        bool running = true;
        sf::Clock tickTimer;
        sf::Time elapsed;
        sf::Event event;

        view.setSize(app.getSize().x, app.getSize().y);
        view.setCenter(app.getSize().x / 2.0f, app.getSize().y / 2.0f);

        while (running) {
            elapsed = tickTimer.restart();
            const double dt = elapsed.asSeconds();

            while (app.pollEvent(event)) {
                process_subscreen_events(app, event);
                // Window closed
                if (event.type == sf::Event::Closed) return (-1);
                if (event.type == sf::Event::MouseWheelMoved) {
                    const int delta = event.mouseWheel.delta;
                    if (delta < 0 && zoom_level < max_zoom) {
                        view.zoom(1 + zoom_speed);
                        zoom_level += 2;
                    } else if (delta > 0 && zoom_level > min_zoom) {
                        view.zoom(1 - zoom_speed);
                        zoom_level -= 2;
                    }
                }
                if (event.type == sf::Event::KeyPressed) {
                    switch (event.key.code) {
                        case sf::Keyboard::Escape:
                            return MenuState::PAUSED;
                        default:
                            break;
                    }
                }
            }

            // this is the non event based input
            handle_input(dt);
            // update all subscreens now that events are done
            update_subscreens(elapsed);
            app.clear(sf::Color::Black);
            app.setView(view);
            app.draw(*world);
            // set back to normal 1080p (util.h) size
            // before drawing the HUD so it is scaled correctly
            app.setView(app.getDefaultView());
            app.draw(hud);
            // output to screen
            app.display();
        }
        return -1;
    }
};

int main() {
    sf::Font font;
    font.loadFromFile("./resources/Roboto-Regular.ttf");
    setGlobalFont(&font);

    load_keys();
    // TODO move export to save options
    export_keys();

    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "Ahoy!");
    window.setFramerateLimit(60);

    GameScreen gs;
    gs.run(window);
    return 0;
}

