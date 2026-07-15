// 文件: campus_rpg.h
// 功能: 校园RPG游戏引擎 - 头文件
// 包含: 所有类的声明、全局变量/常量声明、函数原型声明
// 设计模式: 面向对象(OOP)、继承体系(Item/Card/Treasure)、
//           策略模式(多态apply/use)、工厂模式(clone)
#ifndef CAMPUS_RPG_H//预处理
#define CAMPUS_RPG_H

#include <iostream>
#include <string>
#include <vector>//向量容器
#include <map>//映射容器,键值对
#include <unordered_map>
#include <queue>
#include <random>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <limits>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace std;

// ============================================
// Globals & Utilities
// ============================================
// 【考点6.1 STL::vector】ALL_SUBJECTS/ARTS/SCIENCES 使用vector存储学科常量
// 【考点6.2 STL::map】全局数据使用map管理物品、敌人等映射关系
extern mt19937 rng;

void clear_screen();
void pause_msg();
int rand_int(int lo, int hi);
double rand_float();
//数组索引,模板类
template<typename T>
T rand_choice(const vector<T>& vec) {
    return vec[rand_int(0, (int)vec.size() - 1)];
}
                        
// 【挑战6:STL高级应用】加权随机选择 - 使用discrete_distribution实现权重洗牌
template<typename T>
T weighted_choice(const vector<T>& items, const vector<double>& weights) {
    discrete_distribution<int> dist(weights.begin(), weights.end());
    return items[dist(rng)];
}

// 【挑战6:STL高级应用】卡牌强度评分(1-10), 用于priority_queue调度
extern unordered_map<string,int> CARD_POWER;

// ============================================
// Subject constants
// ============================================
// 【 STL::vector】学科列表常量, 使用vector存储字符串集合
extern const vector<string> ALL_SUBJECTS;
extern const vector<string> ARTS;
extern const vector<string> SCIENCES;
extern const int MIN_SPECIALTY;
extern const int MAX_SPECIALTY;

// ============================================
// Forward declarations
// ============================================
class Player;
class Enemy;
class Item;
class Equipment;
class Card;
class Treasure;
struct BattleContext;

// ============================================
// Challenge 6: STL高级应用 - 卡牌优先级结构体
// 用于priority_queue排序: 按power_level降序, power_level越高优先级越高
// 支持加权抽牌: 高优先级卡牌有更高概率被抽取
// ============================================
struct CardPriority {
    Card* card;
    int power_level;   // 卡牌强度评分(1-10)
    CardPriority(Card* c, int pl) : card(c), power_level(pl) {}
    bool operator<(const CardPriority& other) const {
        return power_level < other.power_level; // 大顶堆: power越大的优先级越高
    }
};

// 成长可视化：数据追踪结构体
// 【新增功能】记录玩家成长历程, 用于生成图表
struct GrowthSnapshot {
    int level, max_hp, atk, dfs;
    string timestamp;
};

struct StageRecord {
    string stage_id, stage_name, timestamp;
    bool cleared;
    int turns_taken;
};

// Item base（物品基类）
// 【考点1.1 封装】成员变量private/protected, 通过public方法访问
// 【考点1.3 多态】虚函数 use()/clone() 实现运行时多态
// 【考点2.2 继承体系根类】Item是Book/Potion/Equipment的父类
class Item {
public:
    string item_id, name, desc, item_type;  // 【注】此处成员为public, 实际工程建议封装为private
    int price;
    Item() : item_type("other"), price(0) {}
    Item(string id, string n, string d, int p, string t = "other")
        : item_id(id), name(n), desc(d), item_type(t), price(p) {}
    virtual ~Item() = default;  // 【考点1.3 多态】虚析构函数确保子类正确析构
    virtual pair<bool,string> use(Player&) { return {false,""}; }   // 【考点1.3 多态】虚函数,子类重写
    virtual Item* clone() const { return new Item(*this); }          // 【考点1.3 多态】原型模式clone
};

// ============================================
// Book（课本类）
// 【考点1.2 继承】Book继承自Item, 复用基类属性与方法
// 【考点1.3 多态】重写use()和clone()实现具体行为
// 【考点2.1 物品类型一】课本类 - 提升学科掌握程度
// ============================================
class Book : public Item {
public:
    string subject;
    Book(string id, string n, string d, int p, string subj)
        : Item(id,n,d,p,"book"), subject(subj) {}
    pair<bool,string> use(Player& player) override;  // 【考点1.3 多态】子类重写虚函数
    Item* clone() const override { return new Book(*this); }
};

// ============================================
// Potion（药品类）
// 【考点1.2 继承】Potion继承自Item
// 【考点2.2 物品类型二】药品类 - 恢复HP/提升属性
// ============================================
class Potion : public Item {
public:
    int hp_restore, atk_boost, dfs_boost;
    Potion(string id, string n, string d, int p, int hp=0, int atk=0, int dfs=0)
        : Item(id,n,d,p,"potion"), hp_restore(hp), atk_boost(atk), dfs_boost(dfs) {}
    pair<bool,string> use(Player& player) override;  // 【考点1.3 多态】重写use
    Item* clone() const override { return new Potion(*this); }
};

// ============================================
// Equipment（装备类）
// 【考点1.2 继承】Equipment继承自Item
// 【考点2.3 物品类型三】装备类 - 衣服/文具, 提供属性加成和特殊效果
// 【考点1.4 类间关联】Equipment与Player通过apply_bonus/remove_bonus关联
// ============================================
class Equipment : public Item {
public:
    string equip_type, subject_bonus, special_effect, rarity;
    string prev_subject_mastery;
    int atk_bonus, dfs_bonus, hp_bonus, exp_bonus, shop_level;
    bool damage_true_flag;
    int ally_atk_bonus;
    string ally_subject_master;
    int star_pen_bonus;
    int hope_heal_pct;

    Equipment() : rarity("白色"),
                  atk_bonus(0), dfs_bonus(0), hp_bonus(0), exp_bonus(0), shop_level(1),
                  damage_true_flag(false), ally_atk_bonus(0),
                  star_pen_bonus(0), hope_heal_pct(0) {}
    Equipment(string id, string n, string d, int p, string eq_type,
              int atk_b=0, int dfs_b=0, int hp_b=0, string subj_b="",
              int exp_b=0, bool dt=false, int ally_atk=0, string ally_subj="",
              string se="", int sl=1, string rar="白色", int spb=0, int hhp=0)
        : Item(id,n,d,p,"equipment_"+eq_type), equip_type(eq_type),
          subject_bonus(subj_b), special_effect(se), rarity(rar),
          atk_bonus(atk_b), dfs_bonus(dfs_b), hp_bonus(hp_b), exp_bonus(exp_b), shop_level(sl),
          damage_true_flag(dt), ally_atk_bonus(ally_atk), ally_subject_master(ally_subj),
          star_pen_bonus(spb), hope_heal_pct(hhp) {}
    pair<bool,string> use(Player& player) override;
    void apply_bonus(Player& player);
    void remove_bonus(Player& player);
    Item* clone() const override { return new Equipment(*this); }
};

// ============================================
// Enemy（敌人类）
// 【考点3.2 必须类设计】敌人类 - 代表战斗中的敌人
// 【考点3.1 封装】属性成员通过public访问(建议工程中改为private+getter)
// 【考点4.5 战斗系统】敌人支持技能(skills)、护盾(shield)、坚韧等机制
// 【考点6.1 STL::vector】skills使用vector存储技能名称列表
// 【考点6.2 STL::map】card_buffs使用vector<map<string,string>>存储buff
// ============================================
class Enemy {
public:
    string enemy_id, name, subject_tag;
    int max_hp, hp, atk_val, dfs_val, exp_reward, gold_reward;
    vector<string> skills;                    // 【考点6.1 STL::vector】技能列表
    bool is_boss;
    int shield;
    vector<map<string,string>> card_buffs;    // 【考点6.2 STL::map】buff键值对
    bool survive_used;
    int immune_turns;

    Enemy() : max_hp(0), hp(0), atk_val(0), dfs_val(0), exp_reward(0), gold_reward(0),
              is_boss(false), shield(0), survive_used(false), immune_turns(0) {}
    Enemy(string eid, string n, string stag, int h, int a, int d,
          int exp_r=0, int gold_r=0, vector<string> sk={}, bool boss=false)
        : enemy_id(eid), name(n), subject_tag(stag), max_hp(h), hp(h),
          atk_val(a), dfs_val(d), exp_reward(exp_r), gold_reward(gold_r),
          skills(sk), is_boss(boss), shield(0), survive_used(false), immune_turns(0) {}
    bool is_alive() const { return hp > 0; }
};

Enemy create_stage_enemy(const Enemy& base, int level);

// Card base（卡牌基类）
// 【考点1.2 继承体系】Card是10种子卡牌的父类
// 【考点1.3 多态】虚函数apply()被子类重写实现不同效果
class Card {
public:
    string card_id, name, desc, effect_type, effect_desc;
    int effect_value;
    Card() : effect_value(0) {}
    Card(string cid, string n, string d, string et, int ev=0, string ed="")
        : card_id(cid), name(n), desc(d), effect_type(et),
          effect_desc(ed.empty()?d:ed), effect_value(ev) {}
    virtual ~Card() = default;
    virtual pair<bool,string> apply(Player& player, BattleContext* ctx=nullptr) { return {false,""}; }  // 【考点1.3 多态】
    virtual Card* clone() const { return new Card(*this); }
};

// Quest（任务类）
// 【考点3.2 必须类设计】任务类 - 包含任务名/描述/完成条件/奖励
// 【考点4.4 任务系统】支持查看/接受/完成任务/领取奖励
class Quest {
public:
    string quest_id, name, desc, quest_type;
    int exp_reward, gold_reward, target_count, progress;
    string item_reward;
    bool completed;

    Quest() : exp_reward(0), gold_reward(0), target_count(1), progress(0), completed(false) {}
    Quest(string qid, string n, string d, string qt, int exp_r=0, int gold_r=0,
          string item="", int tc=1)
        : quest_id(qid), name(n), desc(d), quest_type(qt), exp_reward(exp_r),
          gold_reward(gold_r), target_count(tc), progress(0), item_reward(item), completed(false) {}
    void reset() { progress=0; completed=false; }
    bool is_completed() const { return progress>=target_count; }
    string get_reward_desc() const;
};
// Treasure base（秘宝/藏品基类）
// 【考点1.2 继承体系】Treasure是6种子藏品的父类
// 【考点1.3 多态】apply()/remove()被子类重写
class Treasure {
public:
    string treasure_id, name, desc, effect_type, effect_desc;
    int effect_value;
    Treasure() : effect_value(0) {}
    Treasure(string tid, string n, string d, string et, int ev=0, string ed="")
        : treasure_id(tid), name(n), desc(d), effect_type(et),
          effect_desc(ed.empty()?d:ed), effect_value(ev) {}
    virtual ~Treasure() = default;
    virtual string apply(Player& player) { return ""; }   // 【考点1.3 多态】
    virtual void remove(Player&) {}                        // 【考点1.3 多态】
    virtual Treasure* clone() const { return new Treasure(*this); }
};
// BattleContext
struct BattleContext {
    int turn = 0;
    map<string,int> yachiyo;
    int yachiyo_energy = 0;
    int yachiyo_max_energy = 3;
    bool has_yachiyo() const { return yachiyo.count("hp") && yachiyo.at("hp") > 0; }
    bool can_yachiyo_ultimate() const { return yachiyo_energy >= yachiyo_max_energy; }

    int tian_xian_turns = 0;
    int tian_xian_hp_buff = 0;
    int tian_xian_dfs_buff = 0;
    int tian_xian_orig_max_hp = 0;

    int pi_ya_turns = 0;

    double attack_qi_acc = 0.0;
    bool escaped = false;

    bool has_pi_ya_turns() const { return pi_ya_turns > 0; }
};

// Player class（角色类）
// 【考点3.2 必须类设计】角色类 - 核心类, 包含所有角色属性
// 【考点4.1 角色管理】创建角色/查看信息/属性管理/存档读取
// 【考点4.6 等级成长】经验值累计/等级提升/属性增长
// 【考点4.5 战斗系统】通过atk()/dfs()/gain_qi()等参与战斗
// 【考点6.2 STL::map】subjects(学科掌握), equipment(装备), inventory(背包)
// 【考点6.1 STL::vector】completed_quests, treasures, card_buffs
// 【考点5 数据持久化】save()/load()实现文件存档
// 【考点1.4 类间关联】与Equipment/Quest/Enemy/Item等多个类有交互
class Player {
public:
    string name;
    int level, exp, exp_to_next, gold, max_hp, hp, base_atk, base_dfs;
    double atk_mult, dfs_mult, exp_mult, ally_atk_mult, gold_mult;
    bool damage_true, has_revive;
    map<string,string> subjects;          // 【考点6.2 STL::map】学科→掌握程度映射
    string track;
    int qi, max_qi, qi_overflow;
    map<string,Equipment*> equipment;     // 【考点6.2 STL::map】装备槽位→装备
    map<string,int> inventory;            // 【考点6.2 STL::map】物品ID→数量
    vector<map<string,string>> card_buffs; // 【考点6.1 STL::vector】卡牌buff列表
    unordered_map<string,int> card_buff_lookup; // 【挑战6:unordered_map】O(1)卡牌buff类型查找
    int shield;
    bool survive_used;
    bool uniform_survive_used;
    int immune_turns;
    vector<string> completed_quests;      // 【考点6.1 STL::vector】已完成任务ID列表
    vector<string> treasures;             // 【考点6.1 STL::vector】已获得秘宝名称列表
    unordered_map<string,int> subject_stats; // 【挑战6:unordered_map】学科战斗统计(伤害/击杀)
    
    // 【新增】成长可视化数据追踪
    vector<GrowthSnapshot> growth_snapshots;
    vector<StageRecord> stage_records;
    map<string,int> item_usage_count;
    int total_battles;
    int total_wins;

    Player(string n = "无名学生");
    ~Player();

    int atk() const;                      // 【考点1.1 封装】通过方法访问计算后的攻击值
    int dfs() const;                      // 【考点1.1 封装】通过方法访问计算后的防御值
    bool is_alive() const;

    int get_subject_mastery(const string& subj) const;
    void add_item(const string& id, int qty=1);
    bool remove_item(const string& id, int qty=1);

    void equip(Equipment* item);
    Equipment* unequip(const string& slot);

    pair<bool,int> gain_exp(int amount);  // 【考点4.6 等级成长】经验累计+自动升级
    void level_up();                       // 【考点4.6 等级成长】等级提升+属性增长
    void gain_qi(int amount=1);
    bool consume_qi(int amount);

    void add_card_buff(const string& type, const string& name, int value);
    bool has_card_buff_type(const string& type) const;
    void rebuild_card_buff_lookup();    // 【挑战6:unordered_map】重建O(1)查找索引

    vector<map<string,string>> get_available_skills() const;

    void setup_initial_subjects(const string& track_, const vector<string>& specifics);
    void reset_battle_state();
    void apply_initial_equipment();
    void display() const;

    void save(const string& filename);         // 【考点5 数据持久化】序列化存档
    static Player load(const string& filename); // 【考点5 数据持久化】反序列化读档
    
    // 【新增】成长可视化数据记录方法
    void record_growth_snapshot();
    void record_stage_clear(const string& stage_id, bool cleared, int turns);
    void record_item_usage(const string& item_id);
};

// ============================================
// Global data storage
extern map<string, Item*> ITEMS;
extern map<string, Enemy> ENEMIES_MAP;
extern map<string, string> SUBJECT_ENEMIES;
extern vector<Card*> CARDS;
extern vector<Treasure*> TREASURES;
extern vector<Quest> QUESTS;
extern vector<string> SHOP_ITEMS;
extern vector<string> PHARMACY_ITEMS;
extern vector<string> SHOP_LEVEL1;
extern vector<string> SHOP_LEVEL2;
extern vector<string> SHOP_LEVEL3;
extern vector<string> SHOP_LEVEL4;

// 【新增】成长可视化面板
extern string generate_dashboard_html(const Player& player);
extern void open_in_browser(const string& filepath);

// ============================================
// Card Subclasses
// ============================================
struct Card1SubjectUpgrade : Card {
    Card1SubjectUpgrade();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card2PercentTrueDamage : Card {
    Card2PercentTrueDamage();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card3LifeStealPct : Card {
    Card3LifeStealPct();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card4DamageReduction : Card {
    Card4DamageReduction();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card5EnergyReturn : Card {
    Card5EnergyReturn();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card6DefendImmune : Card {
    Card6DefendImmune();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card7EnergyEfficiency : Card {
    Card7EnergyEfficiency();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card8SurviveCharge : Card {
    Card8SurviveCharge();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card9UltimateDebuff : Card {
    Card9UltimateDebuff();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

struct Card10ReflectDamage : Card {
    Card10ReflectDamage();
    pair<bool,string> apply(Player& player, BattleContext* = nullptr) override;
};

// ============================================
// Treasure Subclasses
// ============================================
struct Treasure1DfsBoost : Treasure {
    Treasure1DfsBoost();
    string apply(Player& player) override;
    void remove(Player& player) override;
};

struct Treasure2ExpGoldBoost : Treasure {
    Treasure2ExpGoldBoost();
    string apply(Player& player) override;
    void remove(Player& player) override;
};

struct Treasure3ReviveTruePath : Treasure {
    Treasure3ReviveTruePath();
    string apply(Player& player) override;
    void remove(Player& player) override;
};

struct Treasure4AllyBoost : Treasure {
    Treasure4AllyBoost();
    string apply(Player& player) override;
    void remove(Player& player) override;
};

struct Treasure5RandomNotebook : Treasure {
    Treasure5RandomNotebook();
    string apply(Player& player) override;
    void remove(Player& player) override;
};

struct Treasure6YachiyoSummon : Treasure {
    Treasure6YachiyoSummon();
    string apply(Player& player) override;
    void remove(Player& player) override;
};

// ============================================
// Challenge 7: 人工智能技术 - AIDecisionEngine
// 智能敌方决策系统: 根据战斗状态分析最优行动
// 决策因素: 玩家HP%、玩家气能量、敌方HP%、可用技能、buff存在性
// 使用决策树+加权评分, 比固定概率更智能
// 备选: 可扩展LLM API接口(需网络支持)
// ============================================
class AIDecisionEngine {
public:
    // 决策结果枚举
    enum Action { ATTACK, SKILL, DEFEND, HEAL, TRUE_DMG, TARGET_SUMMON, ATTACK_PLAYER };
    // 战斗状态快照
    struct BattleSnapshot {
        int player_hp_pct;       // 玩家血量百分比
        int player_qi;           // 玩家当前气
        bool player_has_reflect; // 玩家是否有反弹
        bool player_immuned;     // 玩家是否免疫
        int enemy_hp_pct;        // 敌人自身血量百分比
        bool enemy_is_boss;      // 是否为Boss
        bool enemy_has_aoe;      // 是否有AOE技能
        bool enemy_has_heal;     // 是否有治疗技能
        bool enemy_has_td;       // 是否有真实伤害
        bool yachiyo_present;    // 月见八千代是否在场
        int yachiyo_hp_pct;      // 八千代血量百分比
        int turn_count;          // 当前回合数
    };
    // LLM API配置(扩展预留)
    struct LLMConfig {
        bool enabled = false;
        string api_url;
        string api_key;
        string model = "deepseek-chat";
    };
    
    Action decide(const BattleSnapshot& snap);
    string get_action_reason() const { return decision_reason; }
    void enable_llm(const LLMConfig& cfg);
    void disable_llm();
    bool is_llm_enabled() const { return llm_config.enabled; }
    
    // 【挑战7扩展】可选的LLM API决策接口(需curl支持)
    static string call_llm_api(const LLMConfig& cfg, const BattleSnapshot& snap);
    
private:
    string decision_reason;
    LLMConfig llm_config;
    // 评分函数
    int score_attack(const BattleSnapshot& snap);
    int score_skill(const BattleSnapshot& snap);
    int score_defend(const BattleSnapshot& snap);
    int score_heal(const BattleSnapshot& snap);
    int score_target_summon(const BattleSnapshot& snap);
};

// ============================================
// Challenge 2: 多线程技术 - AutoSaveSystem
// 后台自动存档: 独立线程定时执行保存
// 使用mutex保护player数据, atomic标志控制启停
// ============================================
class AutoSaveSystem {
    Player* player;
    thread worker;
    mutex mtx;
    atomic<bool> running{false};
    atomic<bool> save_needed{false};
    chrono::seconds interval{30};  // 每30秒自动保存
public:
    AutoSaveSystem(Player* p) : player(p) {}
    ~AutoSaveSystem() { stop(); }
    void start();
    void stop();
    void trigger_save() { save_needed = true; }
    void do_save();  // public so main thread can also call it
private:
    void worker_loop();
};

// ============================================
// BattleSystem（战斗系统类）
// 【考点3.2 必须类设计】游戏管理类之一
// 【考点4.5 战斗系统】选择敌人/攻击受击/HP变化/战斗结果判定/奖励
// 【考点6.1 STL::vector】enemies存储战斗中的敌人列表
// 【考点6.2 STL::map】stage_effects存储关卡效果标志
// 【挑战2:多线程】battle_timer_start用于回合限时
// 【挑战7:AI】ai_engine用于智能敌方决策
// ============================================
class BattleSystem {
    Player* player;                       // 【考点1.4 类间关联】与Player关联
    BattleContext* ctx;
    vector<Enemy> enemies;                // 【考点6.1 STL::vector】敌人列表
    map<string,bool> stage_effects;       // 【考点6.2 STL::map】关卡效果
    bool is_player_turn;
    AIDecisionEngine ai_engine;           // 【挑战7:AI】智能决策引擎
    chrono::steady_clock::time_point battle_timer_start;  // 【挑战2】回合计时
    bool battle_timer_enabled = true;     // 【挑战2】是否启用回合限时

    bool has_energy_efficiency();
    int get_energy_refund(int cost);
    int select_target();
    string choose_attack_subject();
    int calc_damage(Enemy* attacker, const string& subject, bool is_skill=false, double multiplier=1.0);
    int apply_ultimate_debuff(int damage);
    void apply_damage(Enemy* target, int damage, Enemy* attacker, bool is_enemy_damaging);

    void player_normal_attack();
    void player_normal_attack_aoe();
    void player_skill_attack();
    void player_use_item();
    void player_use_card();
    bool player_use_ultimate();
    void player_defend();

    void ultimate_tian_dong_wan_xiang();
    void ultimate_bi_po_shi_fang();
    void ultimate_pi_xing_dai_yue();
    void process_buff_expiry();

    bool can_summon_yachiyo();
    void summon_yachiyo();
    void yachiyo_ultimate();
    void enemy_phase();
    void enemy_use_skill(Enemy& e);
    bool try_escape();
    bool check_battle_end();
    void battle_victory();
    void battle_defeat();
    void end_of_round();
    void battle_loop();
    void player_phase();
    
    // 【挑战2:多线程】战斗计时系统
    void start_battle_timer();
    bool is_battle_timer_expired(int seconds);
    
    // 【挑战7:AI】构建战斗状态快照供AI决策
    AIDecisionEngine::BattleSnapshot build_ai_snapshot(const Enemy& e);

public:
    BattleSystem(Player* p);
    ~BattleSystem();
    void start_battle(vector<Enemy> enems, map<string,bool> effects={});
};

// ============================================
// InventorySystem（背包管理系统）
// 【考点3.2 必须类设计】游戏管理类
// 【考点4.2 背包管理】获得物品/查看背包/使用物品/删除物品
// 【考点6.2 STL::map】item_index序号→(ID,物品指针)映射
// 【考点1.4 类间关联】通过player指针与Player关联
// ============================================
class InventorySystem {
    Player* player;                            // 【考点1.4 类间关联】
    map<int,pair<string,Item*>> item_index;    // 【考点6.2 STL::map】展示用索引
public:
    InventorySystem(Player* p) : player(p) {}
    void view_items();    // 【考点4.2 背包管理】查看背包
    void use_item();      // 【考点4.2 背包管理】使用物品
    void delete_item();   // 【考点4.2 背包管理】删除物品
    void run();
};

// ============================================
// ShopSystem（商店系统）
// 【考点3.2 必须类设计】商店类
// 【考点4.3 商店系统】查看商品/购买商品/出售物品/金币结算/药房
// 【考点1.4 类间关联】通过player指针与Player关联
// ============================================
class ShopSystem {
    Player* player;                            // 【考点1.4 类间关联】
    map<int,pair<string,Item*>> shop_idx;      // 【考点6.2 STL::map】货架索引
    string get_rarity_color(const string& rarity);
public:
    ShopSystem(Player* p) : player(p) {}
    void view_items(int level_filter=-1);   // 【考点4.3 商店系统】查看商品列表
    void buy_item();                         // 【考点4.3 商店系统】购买商品
    void sell_item();                        // 【考点4.3 商店系统】出售物品
    void pharmacy();                         // 【考点4.3 商店系统】药房(药品购买)
    void run();
};

// ============================================
// QuestSystem（任务系统）
// 【考点3.2 必须类设计】游戏管理类
// 【考点4.4 任务系统】查看任务/接受任务/完成任务/领取奖励
// 【考点6.1 STL::vector】QUESTS全局任务列表
// ============================================
class QuestSystem {
    Player* player;                   // 【考点1.4 类间关联】
public:
    QuestSystem(Player* p) : player(p) {}
    void view();                       // 【考点4.4 任务系统】查看任务
    void accept();                     // 【考点4.4 任务系统】接受任务
    void do_quest(Quest& q);          // 【考点4.4 任务系统】执行任务
    void complete_quest(Quest& q);    // 【考点4.4 任务系统】完成任务+奖励
    void run();
};

// ============================================
// GrowthSystem（等级成长系统）
// 【考点3.2 必须类设计】游戏管理类
// 【考点4.6 等级成长】经验值累计/等级提升/属性增长/成长展示
// ============================================
class GrowthSystem {
    Player* player;
public:
    GrowthSystem(Player* p) : player(p) {}
    void view_exp();   // 【考点4.6 等级成长】查看经验值与等级
    void run();
};

// ============================================
// StageSystem（关卡系统）
// 【考点3.2 必须类设计】游戏管理类, 协调战斗/补给/通关
// 【考点1.4 类间关联】组合BattleSystem, 关联Player
// 【考点4.5 战斗系统】耦合-通过BattleSystem进行战斗
// ============================================
class StageSystem {
    Player* player;
    BattleSystem battle_sys;            // 【考点1.4 类间关联】组合关系
    map<string,bool> stage_progress;     // 【考点6.2 STL::map】关卡进度

    void view_stages();
    void view_cards_and_treasures();
    bool has_treasure(const string& name);
    bool has_card_type(const string& card_type);
    bool fight_wave(map<string,bool>& cfg, int level, int wave);
    bool fight_boss(map<string,bool>& cfg, int level);
    void rest_point(int wave);
    void start_stage(const string& stage_id);
    void sweep_stage(const string& stage_id);

public:
    StageSystem(Player* p) : player(p), battle_sys(p) {}
    void run();
};

// ============================================
// Game initialization & main flow
// ============================================
void init_game_data();
void print_title();
void print_rules();
Player create_character();
Player load_or_create();
void main_menu(Player& player);

// 【挑战6:STL高级应用】使用priority_queue的加权抽牌
Card* draw_weighted_card(Player& player);

// 【挑战2:多线程】后台存档入口函数
void auto_save_worker(Player* player, atomic<bool>* running, mutex* mtx, chrono::seconds interval);

#endif // CAMPUS_RPG_H
