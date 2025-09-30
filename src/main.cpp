#include <SDL2/SDL.h>
#include <SDL2/SDL_filesystem.h>
#include <array>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "resource_manager.h"
#include "building_manager.h"

using json = nlohmann::json;

static void draw_bar(SDL_Renderer* r, int x, int y, int w, int h, double v, double vmin, double vmax) {
    SDL_Rect bg{ x, y, w, h };
    SDL_SetRenderDrawColor(r, 34, 32, 42, 255);
    SDL_RenderFillRect(r, &bg);
    if (vmax <= vmin) vmax = vmin + 1.0;
    double t = (v - vmin) / (vmax - vmin);
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    SDL_Rect fg{ x + 2, y + 2, int((w - 4) * t), h - 4 };
    uint8_t rCol = static_cast<uint8_t>(120 - t * 40.0);
    uint8_t gCol = static_cast<uint8_t>(120 + t * 110.0);
    uint8_t bCol = static_cast<uint8_t>(90 + t * 40.0);
    SDL_SetRenderDrawColor(r, rCol, gCol, bCol, 255);
    SDL_RenderFillRect(r, &fg);
    SDL_SetRenderDrawColor(r, 90, 88, 110, 255);
    SDL_RenderDrawRect(r, &bg);
}

static void draw_panel(SDL_Renderer* r, const SDL_Rect& rect, SDL_Color fill, SDL_Color border) {
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(r, &rect);
    SDL_SetRenderDrawColor(r, border.r, border.g, border.b, border.a);
    SDL_RenderDrawRect(r, &rect);
}

constexpr uint8_t make_row(const char (&bits)[6]) {
    uint8_t value = 0;
    for (int i = 0; i < 5; ++i) {
        value = static_cast<uint8_t>((value << 1) | (bits[i] == '1' ? 1 : 0));
    }
    return value;
}

struct GlyphDef {
    char ch;
    std::array<uint8_t, 7> rows;
};

constexpr GlyphDef FONT_TABLE[] = {
    { ' ', { make_row("00000"), make_row("00000"), make_row("00000"), make_row("00000"), make_row("00000"), make_row("00000"), make_row("00000") } },
    { '0', { make_row("01110"), make_row("10001"), make_row("10011"), make_row("10101"), make_row("11001"), make_row("10001"), make_row("01110") } },
    { '1', { make_row("00100"), make_row("01100"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("01110") } },
    { '2', { make_row("01110"), make_row("10001"), make_row("00001"), make_row("00010"), make_row("00100"), make_row("01000"), make_row("11111") } },
    { '3', { make_row("01110"), make_row("10001"), make_row("00001"), make_row("00110"), make_row("00001"), make_row("10001"), make_row("01110") } },
    { '4', { make_row("00010"), make_row("00110"), make_row("01010"), make_row("10010"), make_row("11111"), make_row("00010"), make_row("00010") } },
    { '5', { make_row("11111"), make_row("10000"), make_row("11110"), make_row("00001"), make_row("00001"), make_row("10001"), make_row("01110") } },
    { '6', { make_row("00110"), make_row("01000"), make_row("10000"), make_row("11110"), make_row("10001"), make_row("10001"), make_row("01110") } },
    { '7', { make_row("11111"), make_row("00001"), make_row("00010"), make_row("00100"), make_row("01000"), make_row("01000"), make_row("01000") } },
    { '8', { make_row("01110"), make_row("10001"), make_row("10001"), make_row("01110"), make_row("10001"), make_row("10001"), make_row("01110") } },
    { '9', { make_row("01110"), make_row("10001"), make_row("10001"), make_row("01111"), make_row("00001"), make_row("00010"), make_row("01100") } },
    { 'A', { make_row("01110"), make_row("10001"), make_row("10001"), make_row("11111"), make_row("10001"), make_row("10001"), make_row("10001") } },
    { 'B', { make_row("11110"), make_row("10001"), make_row("10001"), make_row("11110"), make_row("10001"), make_row("10001"), make_row("11110") } },
    { 'C', { make_row("01110"), make_row("10001"), make_row("10000"), make_row("10000"), make_row("10000"), make_row("10001"), make_row("01110") } },
    { 'D', { make_row("11100"), make_row("10010"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("10010"), make_row("11100") } },
    { 'E', { make_row("11111"), make_row("10000"), make_row("10000"), make_row("11110"), make_row("10000"), make_row("10000"), make_row("11111") } },
    { 'F', { make_row("11111"), make_row("10000"), make_row("10000"), make_row("11110"), make_row("10000"), make_row("10000"), make_row("10000") } },
    { 'G', { make_row("01110"), make_row("10001"), make_row("10000"), make_row("10111"), make_row("10001"), make_row("10001"), make_row("01110") } },
    { 'H', { make_row("10001"), make_row("10001"), make_row("10001"), make_row("11111"), make_row("10001"), make_row("10001"), make_row("10001") } },
    { 'I', { make_row("01110"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("01110") } },
    { 'J', { make_row("00001"), make_row("00001"), make_row("00001"), make_row("00001"), make_row("10001"), make_row("10001"), make_row("01110") } },
    { 'K', { make_row("10001"), make_row("10010"), make_row("10100"), make_row("11000"), make_row("10100"), make_row("10010"), make_row("10001") } },
    { 'L', { make_row("10000"), make_row("10000"), make_row("10000"), make_row("10000"), make_row("10000"), make_row("10000"), make_row("11111") } },
    { 'M', { make_row("10001"), make_row("11011"), make_row("10101"), make_row("10101"), make_row("10001"), make_row("10001"), make_row("10001") } },
    { 'N', { make_row("10001"), make_row("10001"), make_row("11001"), make_row("10101"), make_row("10011"), make_row("10001"), make_row("10001") } },
    { 'O', { make_row("01110"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("01110") } },
    { 'P', { make_row("11110"), make_row("10001"), make_row("10001"), make_row("11110"), make_row("10000"), make_row("10000"), make_row("10000") } },
    { 'Q', { make_row("01110"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("10101"), make_row("10010"), make_row("01101") } },
    { 'R', { make_row("11110"), make_row("10001"), make_row("10001"), make_row("11110"), make_row("10100"), make_row("10010"), make_row("10001") } },
    { 'S', { make_row("01111"), make_row("10000"), make_row("10000"), make_row("01110"), make_row("00001"), make_row("00001"), make_row("11110") } },
    { 'T', { make_row("11111"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("00100") } },
    { 'U', { make_row("10001"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("01110") } },
    { 'V', { make_row("10001"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("10001"), make_row("01010"), make_row("00100") } },
    { 'W', { make_row("10001"), make_row("10001"), make_row("10001"), make_row("10101"), make_row("10101"), make_row("10101"), make_row("01010") } },
    { 'X', { make_row("10001"), make_row("10001"), make_row("01010"), make_row("00100"), make_row("01010"), make_row("10001"), make_row("10001") } },
    { 'Y', { make_row("10001"), make_row("10001"), make_row("01010"), make_row("00100"), make_row("00100"), make_row("00100"), make_row("00100") } },
    { 'Z', { make_row("11111"), make_row("00001"), make_row("00010"), make_row("00100"), make_row("01000"), make_row("10000"), make_row("11111") } },
    { '[', { make_row("01110"), make_row("01000"), make_row("01000"), make_row("01000"), make_row("01000"), make_row("01000"), make_row("01110") } },
    { ']', { make_row("01110"), make_row("00010"), make_row("00010"), make_row("00010"), make_row("00010"), make_row("00010"), make_row("01110") } },
    { '-', { make_row("00000"), make_row("00000"), make_row("00000"), make_row("01110"), make_row("00000"), make_row("00000"), make_row("00000") } },
    { ':', { make_row("00000"), make_row("00100"), make_row("00000"), make_row("00000"), make_row("00100"), make_row("00000"), make_row("00000") } },
    { '.', { make_row("00000"), make_row("00000"), make_row("00000"), make_row("00000"), make_row("00000"), make_row("00100"), make_row("00000") } },
    { '?', { make_row("01110"), make_row("10001"), make_row("00010"), make_row("00100"), make_row("00100"), make_row("00000"), make_row("00100") } }
};

const uint8_t* lookup_glyph(char ch) {
    for (const auto& glyph : FONT_TABLE) {
        if (glyph.ch == ch) {
            return glyph.rows.data();
        }
    }
    return nullptr;
}

static std::string sanitize_for_font(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (size_t i = 0; i < input.size();) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c < 0x80) {
            char ch = static_cast<char>(c);
            if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 32);
            if (ch == '\n' || ch == '\r' || ch == '\t') ch = ' ';
            if (ch == '\'' || ch == '`') ch = ' ';
            if (ch < 32 || ch > 126) ch = ' ';
            out.push_back(ch);
            ++i;
            continue;
        }
        auto push_letter = [&](char letter) {
            out.push_back(letter);
        };
        auto push_letters = [&](const char* letters) {
            while (*letters) out.push_back(*letters++);
        };
        if (c == 0xC3 && i + 1 < input.size()) {
            unsigned char next = static_cast<unsigned char>(input[i + 1]);
            switch (next) {
                case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85:
                case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5:
                    push_letter('A'); break;
                case 0x86: case 0xA6:
                    push_letters("AE"); break;
                case 0x87: case 0xA7:
                    push_letter('C'); break;
                case 0x88: case 0x89: case 0x8A: case 0x8B:
                case 0xA8: case 0xA9: case 0xAA: case 0xAB:
                    push_letter('E'); break;
                case 0x8C: case 0x8D: case 0x8E: case 0x8F:
                case 0xAC: case 0xAD: case 0xAE: case 0xAF:
                    push_letter('I'); break;
                case 0x91: case 0xB1:
                    push_letter('N'); break;
                case 0x92: case 0x93: case 0x94: case 0x95: case 0x96:
                case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6:
                    push_letter('O'); break;
                case 0x98: case 0xB8:
                    push_letter('Y'); break;
                case 0x99: case 0x9A: case 0x9B: case 0x9C:
                case 0xB9: case 0xBA: case 0xBB: case 0xBC:
                    push_letter('U'); break;
                case 0x9D: case 0xBD:
                    push_letter('U'); break;
                case 0x9F: case 0xBF:
                    push_letter('Y'); break;
                default:
                    push_letter(' '); break;
            }
            i += 2;
            continue;
        }
        if (c == 0xC5 && i + 1 < input.size()) {
            unsigned char next = static_cast<unsigned char>(input[i + 1]);
            if (next == 0x92 || next == 0x93) {
                push_letters("OE");
            } else {
                push_letter(' ');
            }
            i += 2;
            continue;
        }
        if (c == 0xE2 && i + 2 < input.size()) {
            unsigned char n1 = static_cast<unsigned char>(input[i + 1]);
            unsigned char n2 = static_cast<unsigned char>(input[i + 2]);
            if (n1 == 0x80 && (n2 == 0x99 || n2 == 0x98)) {
                push_letter(' ');
                i += 3;
                continue;
            }
        }
        push_letter(' ');
        ++i;
    }
    return out;
}

void draw_text(SDL_Renderer* r, int x, int y, const std::string& text, SDL_Color color, int scale = 2) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    std::string prepared = sanitize_for_font(text);
    int cursor = x;
    for (unsigned char ch : prepared) {
        const uint8_t* rows = lookup_glyph(static_cast<char>(ch));
        if (!rows) {
            rows = lookup_glyph('?');
            if (!rows) {
                cursor += 6 * scale;
                continue;
            }
        }
        for (int row = 0; row < 7; ++row) {
            uint8_t bits = rows[row];
            for (int col = 0; col < 5; ++col) {
                if (bits & (1u << (4 - col))) {
                    SDL_Rect pixel{ cursor + col * scale, y + row * scale, scale, scale };
                    SDL_RenderFillRect(r, &pixel);
                }
            }
        }
        cursor += 6 * scale;
    }
}

static std::string format_quantity(double v) {
    double absV = std::fabs(v);
    if (absV < 0.0005) return "0";
    std::ostringstream os;
    if (absV >= 1000.0) {
        os << std::fixed << std::setprecision(0) << std::round(absV);
    } else if (absV >= 10.0) {
        os << std::fixed << std::setprecision(1) << absV;
    } else {
        os << std::fixed << std::setprecision(2) << absV;
    }
    return os.str();
}

static std::string format_signed(double v) {
    if (std::fabs(v) < 0.0005) return "0";
    std::ostringstream os;
    if (v > 0.0) os << '+';
    else if (v < 0.0) os << '-';
    os << format_quantity(std::fabs(v));
    return os.str();
}

static std::string join_costs(const std::vector<Cost>& costs) {
    if (costs.empty()) return "--";
    std::ostringstream os;
    for (size_t i = 0; i < costs.size(); ++i) {
        if (i > 0) os << ", ";
        os << costs[i].res << ' ' << format_quantity(costs[i].qty);
    }
    return os.str();
}

static std::string join_rates(const std::vector<Cost>& rates, int count, bool isOutput) {
    if (rates.empty() || count <= 0) return "--";
    std::ostringstream os;
    for (size_t i = 0; i < rates.size(); ++i) {
        if (i > 0) os << ", ";
        double perSecond = rates[i].qty * static_cast<double>(count);
        if (!isOutput) perSecond = -perSecond;
        os << rates[i].res << ' ' << format_signed(perSecond) << "/s";
    }
    return os.str();
}

static bool read_json_file(const std::string& path, json& out) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::printf("Erreur: impossible d'ouvrir %s\n", path.c_str());
        return false;
    }
    try {
        f >> out;
    } catch (const std::exception& ex) {
        std::printf("Erreur: lecture JSON %s (%s)\n", path.c_str(), ex.what());
        return false;
    }
    return true;
}

void loadResources(ResourceManager& rm, const std::string& path) {
    json jr;
    if (!read_json_file(path, jr)) return;
    for (auto& r : jr) {
        const std::string id = r.value("id", "");
        if (id.empty()) continue;
        Resource& res = rm.ensureResource(id);
        res.id = id;
        res.qty = r.value("qty", 0.0);
        res.qmin = r.value("qmin", 0.0);
        res.qmax = r.value("qmax", 0.0);
    }
}

void loadBuildings(BuildingManager& bm, const std::string& path) {
    json jb;
    if (!read_json_file(path, jb)) return;
    for (auto& b : jb) {
        const std::string id = b.value("id", "");
        const std::string name = b.value("name", id);
        std::vector<Cost> cost, inputs, outputs;
        if (b.contains("cost")) {
            for (auto& c : b["cost"]) {
                cost.push_back({ c.value("res", std::string{}), c.value("qty", 0.0) });
            }
        }
        if (b.contains("inputs")) {
            for (auto& i : b["inputs"]) {
                inputs.push_back({ i.value("res", std::string{}), i.value("qty", 0.0) });
            }
        }
        if (b.contains("outputs")) {
            for (auto& o : b["outputs"]) {
                outputs.push_back({ o.value("res", std::string{}), o.value("qty", 0.0) });
            }
        }
        if (!id.empty()) {
            bm.addPrototype(Building(id, name, cost, outputs, inputs));
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) return 1;
    SDL_Window* win = SDL_CreateWindow("Medieval Idle",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, 0);
    if (!win) { SDL_Quit(); return 2; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) { SDL_DestroyWindow(win); SDL_Quit(); return 3; }

    ResourceManager rm;
    BuildingManager bm;

    std::filesystem::path dataDir;
    if (char* base = SDL_GetBasePath()) {
        dataDir = std::filesystem::path(base);
        SDL_free(base);
    } else {
        dataDir = std::filesystem::current_path();
    }
    dataDir /= "data";

    loadResources(rm, (dataDir / "resources.json").string());
    loadBuildings(bm, (dataDir / "buildings.json").string());

    for (const auto& proto : bm.prototypes) {
        for (const auto& c : proto.base_cost) {
            if (!c.res.empty()) {
                Resource& res = rm.ensureResource(c.res);
                if (res.id.empty()) res.id = c.res;
            }
        }
        for (const auto& c : proto.inputs) {
            if (!c.res.empty()) {
                Resource& res = rm.ensureResource(c.res);
                if (res.id.empty()) res.id = c.res;
            }
        }
        for (const auto& c : proto.outputs) {
            if (!c.res.empty()) {
                Resource& res = rm.ensureResource(c.res);
                if (res.id.empty()) res.id = c.res;
            }
        }
    }

    if (rm.resources.empty()) {
        std::printf("Avertissement: aucune ressource chargee depuis %s\n", (dataDir / "resources.json").string().c_str());
    }
    if (bm.prototypes.empty()) {
        std::printf("Avertissement: aucun batiment charge depuis %s\n", (dataDir / "buildings.json").string().c_str());
    }

    const std::array<SDL_Scancode, 36> keyPool = {
        SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5,
        SDL_SCANCODE_6, SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_0,
        SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R, SDL_SCANCODE_T,
        SDL_SCANCODE_Y, SDL_SCANCODE_U, SDL_SCANCODE_I, SDL_SCANCODE_O, SDL_SCANCODE_P,
        SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F, SDL_SCANCODE_G,
        SDL_SCANCODE_H, SDL_SCANCODE_J, SDL_SCANCODE_K, SDL_SCANCODE_L,
        SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V, SDL_SCANCODE_B,
        SDL_SCANCODE_N, SDL_SCANCODE_M
    };

    std::vector<SDL_Scancode> buildingHotkeys;
    std::vector<std::string> buildingKeyLabels;
    buildingHotkeys.reserve(bm.prototypes.size());
    buildingKeyLabels.reserve(bm.prototypes.size());

    for (size_t i = 0; i < bm.prototypes.size(); ++i) {
        SDL_Scancode code = i < keyPool.size() ? keyPool[i] : SDL_SCANCODE_UNKNOWN;
        buildingHotkeys.push_back(code);
        const char* name = SDL_GetScancodeName(code);
        std::string label = (name && *name) ? name : "?";
        buildingKeyLabels.push_back(label);
    }

    if (!bm.prototypes.empty()) {
        std::printf("Raccourcis construction:\n");
        for (size_t i = 0; i < bm.prototypes.size(); ++i) {
            std::printf("  [%s] -> %s\n", buildingKeyLabels[i].c_str(), bm.prototypes[i].name.c_str());
            if (buildingHotkeys[i] == SDL_SCANCODE_UNKNOWN) {
                std::printf("    (Aucun raccourci disponible au dela de %zu batiments)\n", keyPool.size());
                break;
            }
        }
    }

    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;
    const int layoutMargin = 20;
    const int topMargin = 30;
    const int bottomPanelHeight = 110;
    const int resourcePanelWidth = 360;
    const int buildingPanelWidth = 360;
    const int mainAreaHeight = windowHeight - bottomPanelHeight - topMargin - layoutMargin;

    SDL_Rect resourcePanel{ layoutMargin, topMargin, resourcePanelWidth, mainAreaHeight };
    SDL_Rect buildingPanel{ resourcePanel.x + resourcePanel.w + layoutMargin, topMargin, buildingPanelWidth, mainAreaHeight };
    SDL_Rect yardArea{ buildingPanel.x + buildingPanel.w + layoutMargin, topMargin,
        std::max(220, windowWidth - (buildingPanel.x + buildingPanel.w + 2 * layoutMargin)), mainAreaHeight };

    const int yardSlotSize = 64;
    const int yardSlotSpacing = 24;
    const int yardLabelHeight = 36;
    int yardRows = std::max(1, (yardArea.h - yardLabelHeight) / (yardSlotSize + yardSlotSpacing));
    int yardColumns = std::max(1, static_cast<int>((bm.prototypes.size() + yardRows - 1) / std::max(1, yardRows)));
    int maxYardColumns = std::max(1, (yardArea.w - 60) / (yardSlotSize + yardSlotSpacing));
    if (yardColumns > maxYardColumns) {
        yardColumns = maxYardColumns;
        yardRows = std::max(1, static_cast<int>((bm.prototypes.size() + yardColumns - 1) / std::max(1, yardColumns)));
    }
    const int yardInstancePerRow = 2;
    const int yardInstanceSpacingX = 74;
    const int yardInstanceSpacingY = 74;

    auto anchorForIndex = [&](int index) {
        if (yardArea.w <= 0) return SDL_Point{ yardArea.x, yardArea.y + yardLabelHeight };
        int column = yardColumns > 0 ? index / yardRows : 0;
        int row = yardRows > 0 ? index % yardRows : 0;
        column = std::min(column, std::max(0, yardColumns - 1));
        row = std::min(row, std::max(0, yardRows - 1));
        int x = yardArea.x + 24 + column * (yardSlotSize + yardSlotSpacing);
        int y = yardArea.y + yardLabelHeight + row * (yardSlotSize + yardSlotSpacing);
        return SDL_Point{ x, y };
    };

    const double dt = 0.1;
    double acc = 0.0;
    double title_acc = 0.0;
    auto prev = std::chrono::high_resolution_clock::now();
    bool run = true;

    auto triggerBuild = [&](int index) {
        if (index < 0 || index >= static_cast<int>(bm.prototypes.size())) return;
        SDL_Point anchor = anchorForIndex(index);
        int built = bm.prototypes[index].count;
        int instCol = built % yardInstancePerRow;
        int instRow = built / yardInstancePerRow;
        int x = anchor.x + instCol * yardInstanceSpacingX;
        int y = anchor.y + instRow * yardInstanceSpacingY;
        bm.tryBuild(index, rm, x, y);
    };

    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) run = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) run = false;
            if (e.type == SDL_KEYDOWN) {
                for (size_t i = 0; i < buildingHotkeys.size(); ++i) {
                    if (buildingHotkeys[i] != SDL_SCANCODE_UNKNOWN && e.key.keysym.scancode == buildingHotkeys[i]) {
                        triggerBuild(static_cast<int>(i));
                        break;
                    }
                }
            }
        }

        auto now = std::chrono::high_resolution_clock::now();
        double frame = std::chrono::duration<double>(now - prev).count();
        prev = now;
        acc += frame;
        title_acc += frame;

        int ticks = 0;
        while (acc >= dt && ticks < 10) {
            bm.produceAll(rm, dt);
            auto popIt = rm.resources.find("pop");
            auto foodIt = rm.resources.find("food");
            if (popIt != rm.resources.end() && foodIt != rm.resources.end()) {
                double need = popIt->second.qty * 0.02 * dt;
                if (foodIt->second.qty >= need) {
                    foodIt->second.qty -= need;
                }
            }
            acc -= dt;
            ++ticks;
        }

        if (title_acc >= 0.5) {
            std::ostringstream os;
            os << "Idle";
            if (!rm.order.empty()) {
                os << " | " << std::fixed << std::setprecision(1);
                size_t limit = std::min<size_t>(rm.order.size(), 4);
                for (size_t i = 0; i < limit; ++i) {
                    const auto& id = rm.order[i];
                    const auto* res = rm.find(id);
                    if (!res) continue;
                    if (i > 0) os << ' ';
                    os << id << '=' << res->qty;
                }
            }
            if (!bm.prototypes.empty()) {
                os << " | Build:";
                size_t limit = std::min<size_t>(bm.prototypes.size(), 3);
                for (size_t i = 0; i < limit; ++i) {
                    os << ' ' << buildingKeyLabels[i] << ' ' << bm.prototypes[i].name;
                }
            }
            SDL_SetWindowTitle(win, os.str().c_str());
            title_acc = 0.0;
        }

        SDL_SetRenderDrawColor(ren, 20, 18, 28, 255);
        SDL_RenderClear(ren);

        draw_panel(ren, resourcePanel, SDL_Color{ 28, 28, 40, 255 }, SDL_Color{ 90, 88, 110, 255 });
        SDL_Color labelColor{ 210, 210, 220, 255 };
        const int barHeight = 24;
        const int minBarSpacing = barHeight + 8;
        int barCount = static_cast<int>(rm.order.size());
        int availableBarHeight = resourcePanel.h - 60;
        int barSpacing = barCount > 0 ? std::max(minBarSpacing, availableBarHeight / barCount) : minBarSpacing;
        int barY = resourcePanel.y + 36;
        int barStartX = resourcePanel.x + 140;
        int barWidth = resourcePanel.w - (barStartX - resourcePanel.x) - 24;
        for (const auto& id : rm.order) {
            auto it = rm.resources.find(id);
            if (it == rm.resources.end()) continue;
            const Resource& res = it->second;
            double maxv = res.qmax > 0.0 ? res.qmax : std::max(10.0, res.qty * 1.25 + 5.0);
            std::string label = res.id + " " + format_quantity(res.qty);
            if (res.qmax > 0.0) {
                label += "/";
                label += format_quantity(res.qmax);
            }
            draw_text(ren, resourcePanel.x + 16, barY + 2, label, labelColor, 2);
            draw_bar(ren, barStartX, barY, barWidth, barHeight, res.qty, res.qmin, maxv);
            barY += barSpacing;
        }

        draw_panel(ren, buildingPanel, SDL_Color{ 30, 32, 44, 255 }, SDL_Color{ 100, 96, 120, 255 });
        draw_text(ren, buildingPanel.x + 16, buildingPanel.y + 8, "BATIMENTS", SDL_Color{ 220, 220, 180, 255 }, 2);

        const int cardHeight = 84;
        const int cardSpacing = 10;
        const int columnSpacing = 12;
        int cardAreaTop = buildingPanel.y + 36;
        int cardAreaHeight = buildingPanel.h - (cardAreaTop - buildingPanel.y) - 12;
        int cardsCount = static_cast<int>(bm.prototypes.size());
        int maxCardsPerColumn = cardsCount > 0 ? std::max(1, (cardAreaHeight + cardSpacing) / (cardHeight + cardSpacing)) : 1;
        maxCardsPerColumn = std::max(1, std::min(maxCardsPerColumn, cardsCount > 0 ? cardsCount : 1));
        int columns = cardsCount > 0 ? std::max(1, (cardsCount + maxCardsPerColumn - 1) / maxCardsPerColumn) : 1;
        int cardAreaWidth = buildingPanel.w - 24;
        while (columns > 1) {
            int candidateWidth = (cardAreaWidth - (columns - 1) * columnSpacing) / columns;
            if (candidateWidth >= 150) break;
            ++maxCardsPerColumn;
            columns = std::max(1, (cardsCount + maxCardsPerColumn - 1) / maxCardsPerColumn);
            if (maxCardsPerColumn > cardsCount) break;
        }
        int cardWidth = columns > 0 ? (cardAreaWidth - (columns - 1) * columnSpacing) / columns : cardAreaWidth;
        if (columns > 1 && cardWidth < 150) {
            columns = 1;
            maxCardsPerColumn = cardsCount > 0 ? cardsCount : 1;
            cardWidth = cardAreaWidth;
        } else if (columns <= 1) {
            columns = 1;
            maxCardsPerColumn = cardsCount > 0 ? cardsCount : 1;
            cardWidth = cardAreaWidth;
        }

        std::ostringstream readyList;
        bool firstReady = true;

        for (size_t i = 0; i < bm.prototypes.size(); ++i) {
            Building& proto = bm.prototypes[i];
            auto nextCost = proto.nextCost();
            bool canBuild = proto.canAfford(rm.resources);
            if (canBuild) {
                if (!firstReady) readyList << ", ";
                readyList << proto.name;
                firstReady = false;
            }
            int column = columns > 0 ? static_cast<int>(i) / maxCardsPerColumn : 0;
            int row = maxCardsPerColumn > 0 ? static_cast<int>(i) % maxCardsPerColumn : 0;
            int cardX = buildingPanel.x + 12 + column * (cardWidth + columnSpacing);
            int cardY = cardAreaTop + row * (cardHeight + cardSpacing);
            if (cardY + cardHeight > buildingPanel.y + buildingPanel.h) continue;
            SDL_Rect cardRect{ cardX, cardY, cardWidth, cardHeight };
            SDL_Color cardFill = canBuild ? SDL_Color{ 46, 58, 48, 255 } : SDL_Color{ 60, 44, 44, 255 };
            SDL_Color cardBorder{ 96, 96, 112, 255 };
            draw_panel(ren, cardRect, cardFill, cardBorder);

            SDL_Color headerColor = canBuild ? SDL_Color{ 220, 235, 190, 255 } : SDL_Color{ 230, 170, 170, 255 };
            SDL_Color costColor = canBuild ? SDL_Color{ 200, 210, 220, 255 } : SDL_Color{ 235, 180, 170, 255 };
            SDL_Color bodyColor{ 190, 200, 210, 255 };

            std::ostringstream header;
            header << '[' << buildingKeyLabels[i] << "] " << proto.name << " x" << proto.count;
            int textY = cardRect.y + 8;
            draw_text(ren, cardRect.x + 8, textY, header.str(), headerColor, 2);

            textY += 18;
            draw_text(ren, cardRect.x + 8, textY, "Cout: " + join_costs(nextCost), costColor, 2);

            textY += 18;
            draw_text(ren, cardRect.x + 8, textY, "Conso: " + join_rates(proto.inputs, proto.count, false), bodyColor, 2);

            textY += 18;
            draw_text(ren, cardRect.x + 8, textY, "Prod: " + join_rates(proto.outputs, proto.count, true), bodyColor, 2);
        }

        draw_panel(ren, yardArea, SDL_Color{ 22, 36, 40, 255 }, SDL_Color{ 80, 110, 110, 255 });
        draw_text(ren, yardArea.x + 12, yardArea.y + 8, "PLACEMENTS", SDL_Color{ 190, 210, 210, 255 }, 2);

        for (size_t i = 0; i < bm.prototypes.size(); ++i) {
            SDL_Point anchor = anchorForIndex(static_cast<int>(i));
            SDL_Rect slotRect{ anchor.x, anchor.y, yardSlotSize, yardSlotSize };
            SDL_SetRenderDrawColor(ren, 70, 80, 92, 255);
            SDL_RenderDrawRect(ren, &slotRect);
        }

        bm.render(ren);

        for (size_t i = 0; i < bm.prototypes.size(); ++i) {
            SDL_Point anchor = anchorForIndex(static_cast<int>(i));
            bool canBuild = bm.prototypes[i].canAfford(rm.resources);
            SDL_Color label = canBuild ? SDL_Color{ 220, 235, 210, 255 } : SDL_Color{ 220, 170, 170, 255 };
            draw_text(ren, anchor.x, anchor.y - 18, "[" + buildingKeyLabels[i] + "]", label, 2);
        }

        SDL_Rect hintPanel{ layoutMargin, windowHeight - bottomPanelHeight, windowWidth - 2 * layoutMargin, bottomPanelHeight - 20 };
        draw_panel(ren, hintPanel, SDL_Color{ 30, 30, 40, 255 }, SDL_Color{ 90, 88, 110, 255 });

        std::vector<std::string> hintLines;
        hintLines.push_back("Construisez avec les touches indiquees; appuyez plusieurs fois pour empiler.");
        hintLines.push_back("Cadre vert = ressources suffisantes. Cadre rouge = cout trop eleve.");
        if (firstReady) {
            hintLines.push_back("Pret: --");
        } else {
            hintLines.push_back("Pret: " + readyList.str());
        }
        hintLines.push_back("Esc pour quitter. Les jauges s adaptent a vos stocks.");

        int hintY = hintPanel.y + 16;
        for (const auto& line : hintLines) {
            draw_text(ren, hintPanel.x + 16, hintY, line, SDL_Color{ 200, 205, 220, 255 }, 2);
            hintY += 18;
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}