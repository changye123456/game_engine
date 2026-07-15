// ============================================================
// 文件: campus_rpg.cpp
// 功能: 校园RPG游戏引擎全部功能实现
// 结构: 全局工具函数 → 游戏数据初始化 → 角色类实现 → 卡牌子类 →
//        藏品子类 → 战斗系统 → 背包系统 → 商店系统 → 任务系统 →
//        成长系统 → 关卡系统
// 设计模式: 面向对象(OOP)、工厂模式(clone)、策略模式(卡牌/藏品多态)
// ============================================================
#include "campus_rpg.h"
#include <climits>
#include <ctime>

// ============================================
// 第一部分: 全局工具函数
// 提供清屏、暂停、随机数等通用功能
// ============================================
// 【考点6 挑战任务:STL高级应用】使用random_device/mt19937随机数引擎
// mt19937: 梅森旋转算法, 提供高质量伪随机数, 适合游戏随机性需求
mt19937 rng(random_device{}());

// 【挑战6:STL高级应用】卡牌强度评分(1-10), 用于priority_queue调度和加权抽牌
// power_level越高, 被抽取的概率越大(加权权重=power_level)
unordered_map<string,int> CARD_POWER = {
    {"subject_upgrade",5},{"percent_true_damage",7},{"lifesteal_pct",6},
    {"damage_reduction",6},{"energy_return",5},{"defend_immune",4},
    {"energy_efficiency",8},{"survive_charge",9},{"ultimate_debuff",7},
    {"reflect_damage",10}
};

// ============================================
// clear_screen: 跨平台清屏
// 用途: 清理控制台输出, 提升界面整洁度
// 实现: Windows使用cls命令, Linux/Mac使用clear命令
// ============================================
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// pause_msg: 暂停等待用户输入
// 用途: 防止信息刷屏, 让用户有时间阅读输出
// 实现: 使用getline等待一次回车
void pause_msg() {
    cout << "\n  按回车键继续...";
    string dummy; getline(cin, dummy);
}

// rand_int: 生成[lo, hi]区间内的随机整数
// 使用uniform_int_distribution确保均匀分布
int rand_int(int lo, int hi) {
    if (lo >= hi) return lo;
    uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

// rand_float: 生成[0.0, 1.0)区间内的随机浮点数
// 用于概率判断(如逃跑成功率、物品掉落率)
double rand_float() {
    uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng);
}

// 第二部分: 全局常量定义
// 学科列表常量, 使用vector便于遍历和随机访问
// 【考点6.1 STL::vector】使用vector存储学科列表, 支持遍历和随机访问
// 选择vector的理由: 需要O(1)随机访问(按索引选取学科)和顺序遍历
const vector<string> ALL_SUBJECTS = {
    "语文", "数学", "英语",
    "物理", "化学", "生物",
    "历史", "政治", "地理"
};
const vector<string> ARTS = {"语文", "英语", "历史", "政治", "地理", "数学"};
const vector<string> SCIENCES = {"数学", "物理", "化学", "生物", "英语", "语文"};
const int MIN_SPECIALTY = 1;
const int MAX_SPECIALTY = 6;

// ============================================
// 第三部分: 全局游戏数据存储
// 所有物品、敌人、卡牌、藏品、任务、商品列表
// 使用map实现O(log n)查找, vector实现快速遍历
// ============================================
// 【考点6.2 STL::map】使用map管理所有游戏数据: ID->对象映射, O(log n)查找
// 选择map的理由: 需要根据字符串ID快速查找对象, map提供有序键值对存储
map<string, Item*> ITEMS;                  // 物品字典: 物品ID→物品指针
map<string, Enemy> ENEMIES_MAP;            // 敌人字典
map<string, string> SUBJECT_ENEMIES;       // 学科→敌人ID映射
// 【考点6.1 STL::vector】动态数组, 存储卡牌/藏品/任务/商品列表, 便于遍历和随机抽取
vector<Card*> CARDS;
vector<Treasure*> TREASURES;
vector<Quest> QUESTS;
vector<string> SHOP_ITEMS;
vector<string> PHARMACY_ITEMS;
vector<string> SHOP_LEVEL1, SHOP_LEVEL2, SHOP_LEVEL3, SHOP_LEVEL4;

// ============================================
// Enemy
// ============================================
// 【考点4.5 战斗系统】关卡敌人创建 - 根据关卡等级调整敌人属性
Enemy create_stage_enemy(const Enemy& base, int level) {
    Enemy e = base;
    if (level >= 1) { e.atk_val = (int)(e.atk_val*1.1); e.dfs_val = (int)(e.dfs_val*1.1); }
    if (level >= 2) { e.atk_val = (int)(e.atk_val*0.91); e.dfs_val = (int)(e.dfs_val*0.91); }
    if (level >= 3) { e.atk_val = (int)(e.atk_val*0.91); e.dfs_val = (int)(e.dfs_val*0.91); }
    if (level >= 4) { e.atk_val = (int)(e.atk_val*0.91); e.dfs_val = (int)(e.dfs_val*0.91); }
    if (level >= 5) { e.atk_val = (int)(e.atk_val*0.91); e.dfs_val = (int)(e.dfs_val*0.91); }
    return e;
}

// ============================================
// Item methods
// ============================================
// 【考点1.3 多态】Book重写use() - 提升学科掌握程度
// 【考点2.1 物品类型一 课本】效果: 将"了解"提升为"精通", 或获得"了解"
pair<bool,string> Book::use(Player& player) {
    auto it = player.subjects.find(subject);
    if (it!=player.subjects.end()) {
        if (it->second=="精通") return {false,"你已经精通"+subject+"了！"};
        if (it->second=="了解") { player.subjects[subject]="精通"; return {true,"你对"+subject+"的掌握从「了解」提升为「精通」！"}; }
    }
    player.subjects[subject]="了解";
    return {true,"你学会了"+subject+"（了解）！"};
}

// 【考点1.3 多态】Potion重写use() - 恢复HP/提升属性
// 【考点2.2 物品类型二 药品】效果: 恢复HP/临时提升学识/抗压
pair<bool,string> Potion::use(Player& player) {
    vector<string> r;   // 【考点6.1 STL::vector】收集使用效果描述
    if (hp_restore>0) { int old=player.hp; player.hp=(min)(player.hp+hp_restore,player.max_hp); r.push_back("恢复了"+to_string(player.hp-old)+"点HP！"); }
    if (atk_boost>0) { player.base_atk+=atk_boost; r.push_back("学识临时+"+to_string(atk_boost)+"！"); }
    if (dfs_boost>0) { player.base_dfs+=dfs_boost; r.push_back("抗压临时+"+to_string(dfs_boost)+"！"); }
    string msg;
    for (size_t i=0;i<r.size();i++) { if(i>0)msg+="\n"; msg+=r[i]; }
    return {true,msg.empty()?"此物品没有效果。":msg};
}

// 【考点1.3 多态】Equipment重写use() - 装备不可直接使用, 需通过装备栏
// 【考点2.3 物品类型三 装备】效果: 提升属性/提供特殊buff
//utility库,des密钥生成时使用过
pair<bool,string> Equipment::use(Player& player) {
    return {false,"装备无法直接使用，请在装备栏中装备。"};
}

// 【考点1.4 类间关联】Equipment→Player: 装备施加效果到玩家身上
// 【考点1.1 封装】通过方法操作, 内部管理buff添加逻辑
void Equipment::apply_bonus(Player& player) {
    player.max_hp+=hp_bonus; player.hp+=hp_bonus;
    player.base_atk+=atk_bonus; player.base_dfs+=dfs_bonus;
    if (special_effect == "notebook" && !subject_bonus.empty()) {
        auto it = player.subjects.find(subject_bonus);
        if (it != player.subjects.end()) {
            prev_subject_mastery = it->second;
            player.subjects[subject_bonus] = "精通";
        }
    }
    if (hope_heal_pct > 0) {
        player.card_buffs.push_back({{"type","hope_uniform"},{"value",to_string(hope_heal_pct)},{"name",name}});
    }
    if (star_pen_bonus > 0) {
        player.card_buffs.push_back({{"type","star_pen"},{"value",to_string(star_pen_bonus)},{"name",name}});
    }
    if (special_effect == "uniform_survive") {
        player.card_buffs.push_back({{"type","uniform_survive"},{"value","1"},{"name",name}});
    }
    if (special_effect == "pen_core") {
        player.card_buffs.push_back({{"type","pen_core"},{"value","5"},{"name",name}});
    }
}

// 【考点1.4 类间关联】Equipment→Player: 移除装备时撤回效果
void Equipment::remove_bonus(Player& player) {
    player.max_hp-=hp_bonus; player.hp=(max)(1,player.hp-hp_bonus);
    player.base_atk-=atk_bonus; player.base_dfs-=dfs_bonus;
    if (special_effect == "notebook" && !subject_bonus.empty() && !prev_subject_mastery.empty()) {
        player.subjects[subject_bonus] = prev_subject_mastery;
    }
    if (hope_heal_pct > 0) {
        vector<map<string,string>> nb;
        for (auto& b : player.card_buffs)
            if (!b.count("type")||b["type"]!="hope_uniform") nb.push_back(b);
        player.card_buffs=nb;
    }
    if (star_pen_bonus > 0) {
        vector<map<string,string>> nb;
        for (auto& b : player.card_buffs)
            if (!b.count("type")||b["type"]!="star_pen") nb.push_back(b);
        player.card_buffs=nb;
    }
    if (special_effect == "uniform_survive") {
        vector<map<string,string>> nb;
        for (auto& b : player.card_buffs)
            if (!b.count("type")||b["type"]!="uniform_survive") nb.push_back(b);
        player.card_buffs=nb;
    }
    if (special_effect == "pen_core") {
        vector<map<string,string>> nb;
        for (auto& b : player.card_buffs)
            if (!b.count("type")||b["type"]!="pen_core") nb.push_back(b);
        player.card_buffs=nb;
    }
}

// ============================================
// Quest methods
// ============================================
string Quest::get_reward_desc() const {
    string r = "经验值x" + to_string(exp_reward);
    if (gold_reward>0) r += ", 金币x" + to_string(gold_reward);
    if (!item_reward.empty()) r += ", " + item_reward;
    return r;
}

// ============================================
// Player methods
// ============================================
// 【考点4.1 角色管理】构造函数初始化角色默认属性(等级1/HP100/金币100)
Player::Player(string n)
    : name(n), level(1), exp(0), exp_to_next(100), gold(100),
      max_hp(100), hp(100), base_atk(20), base_dfs(10),
      atk_mult(1.0), dfs_mult(1.0), exp_mult(1.0), ally_atk_mult(1.0), gold_mult(1.0),
      damage_true(false), has_revive(false), qi(0), max_qi(6), qi_overflow(0),
      shield(0), survive_used(false), uniform_survive_used(false), immune_turns(0),
      total_battles(0), total_wins(0) {
    // 记录初始成长快照
    record_growth_snapshot();
}

Player::~Player() { for (auto& p : equipment) delete p.second; }

// 【考点1.1 封装】通过公共方法访问计算后的属性值, 隐藏内部乘算逻辑
int Player::atk() const { return (int)(base_atk * atk_mult); }
int Player::dfs() const { return (int)(base_dfs * dfs_mult); }
bool Player::is_alive() const { return hp > 0; }

int Player::get_subject_mastery(const string& subj) const {
    auto it = subjects.find(subj);
    if (it == subjects.end()) return 0;
    return it->second == "精通" ? 2 : 1;
}

void Player::add_item(const string& id, int qty) { inventory[id] += qty; }

bool Player::remove_item(const string& id, int qty) {
    auto it = inventory.find(id);
    if (it != inventory.end() && it->second >= qty) {
        it->second -= qty;
        if (it->second <= 0) inventory.erase(it);
        return true;
    }
    return false;
}

void Player::add_card_buff(const string& type, const string& name, int value) {
    card_buffs.push_back({{"type",type},{"name",name},{"value",to_string(value)}});
    card_buff_lookup[type]++;  // 【挑战6:unordered_map】O(1)更新索引
}

bool Player::has_card_buff_type(const string& type) const {
    // 【挑战6:unordered_map】O(1)哈希查找, 替代原O(n)线性扫描
    return card_buff_lookup.count(type) && card_buff_lookup.at(type) > 0;
}

void Player::rebuild_card_buff_lookup() {
    card_buff_lookup.clear();
    for (auto& b : card_buffs)
        if (b.count("type")) card_buff_lookup[b["type"]]++;
}

// 【考点1.3 多态】动态转型判断Equipment子类
// 【考点4.1 角色管理】装备: clone装备并施加效果
void Player::equip(Equipment* item) {
    string& slot = item->equip_type;
    auto it = equipment.find(slot);
    if (it!=equipment.end() && it->second) { it->second->remove_bonus(*this); delete it->second; }
    Equipment* copy = dynamic_cast<Equipment*>(item->clone());
    copy->apply_bonus(*this);
    equipment[slot]=copy;
}

Equipment* Player::unequip(const string& slot) {
    auto it = equipment.find(slot);
    if (it!=equipment.end() && it->second) {
        Equipment* item = it->second;
        item->remove_bonus(*this);
        equipment[slot]=nullptr;
        return item;
    }
    return nullptr;
}

// 【考点4.6 等级成长】经验累积+自动检测升级(支持连升多级)
pair<bool,int> Player::gain_exp(int amount) {
    amount = (int)(amount * exp_mult);
    exp += amount;
    bool leveled = false;
    while (exp_to_next > 0 && exp >= exp_to_next) {
        exp -= exp_to_next;
        level_up();
        leveled = true;
    }
    return {leveled, amount};
}

// 【考点4.6 等级成长】升级时属性全面增长: HP+15, 学识+5, 抗压+3
void Player::level_up() {
    level++;
    long long next = (long long)exp_to_next * 3 / 2;
    exp_to_next = next > INT_MAX ? INT_MAX : (int)next;
    max_hp += 15; hp += 15;
    base_atk += 5; base_dfs += 3;
    record_growth_snapshot();  // 【新增】记录成长快照
}

// 【新增】成长可视化: 记录等级提升时的属性快照
void Player::record_growth_snapshot() {
    GrowthSnapshot snap;
    snap.level = level;
    snap.max_hp = max_hp;
    snap.atk = atk();
    snap.dfs = dfs();
    // 生成时间戳
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&now));
    snap.timestamp = buf;
    growth_snapshots.push_back(snap);
}

// 【新增】成长可视化: 记录关卡通关/失败
void Player::record_stage_clear(const string& stage_id, bool cleared, int turns) {
    StageRecord rec;
    rec.stage_id = stage_id;
    rec.cleared = cleared;
    rec.turns_taken = turns;
    // 关卡名称映射
    if(stage_id=="N0") rec.stage_name="日常作业(N0)";
    else if(stage_id=="N1") rec.stage_name="周中测(N1)";
    else if(stage_id=="N2") rec.stage_name="周测(N2)";
    else if(stage_id=="N3") rec.stage_name="月考(N3)";
    else if(stage_id=="N4") rec.stage_name="期中考试(N4)";
    else if(stage_id=="N5") rec.stage_name="期末考试(N5)";
    else rec.stage_name = stage_id;
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&now));
    rec.timestamp = buf;
    stage_records.push_back(rec);
    total_battles++;
    if(cleared) total_wins++;
}

// 【新增】成长可视化: 记录物品使用
void Player::record_item_usage(const string& item_id) {
    item_usage_count[item_id]++;
}

void Player::gain_qi(int amount) { qi = min(qi+amount, max_qi); }
bool Player::consume_qi(int amount) {
    if (qi >= amount) { qi -= amount; return true; }
    return false;
}

vector<map<string,string>> Player::get_available_skills() const {
    vector<map<string,string>> skills;
    for (auto& [subj, mastery] : subjects) {
        if (mastery == "精通") {
            skills.push_back({
                {"name", subj + "专精"},
                {"subject", subj},
                {"cost", "3"},
                {"multiplier", "2.0"},
                {"desc", "使用" + subj + "知识进行强力攻击，消耗3气"}
            });
        }
    }
    return skills;
}

void Player::setup_initial_subjects(const string& track_, const vector<string>& specifics) {
    track = track_;
    const vector<string>* prof = (track=="arts") ? &ARTS : &SCIENCES;
    const vector<string>* other = (track=="arts") ? &SCIENCES : &ARTS;
    for (auto& s : *prof)
        subjects[s] = (find(specifics.begin(),specifics.end(),s)!=specifics.end()) ? "精通" : "了解";
    for (auto& s : *other)
        if(subjects.find(s)==subjects.end()) subjects[s] = "了解";
}

void Player::reset_battle_state() {
    shield=0; qi=0; qi_overflow=0;
    vector<string> keep_types = {
        "percent_true_damage","lifesteal_pct","energy_return","defend_immune",
        "energy_efficiency","survive_charge","ultimate_debuff","reflect_damage",
        "damage_reduction","summon_yachiyo",
        "hope_uniform","star_pen","uniform_survive","pen_core"
    };
    vector<map<string,string>> keep;
    for (auto& b : card_buffs)
        if (b.count("type") && find(keep_types.begin(),keep_types.end(),b["type"])!=keep_types.end())
            keep.push_back(b);
    card_buffs = keep;
    rebuild_card_buff_lookup();  // 【挑战6】重建O(1)查找索引
    survive_used=false; uniform_survive_used=false; immune_turns=0; hp=max_hp;
}

void Player::apply_initial_equipment() {
    auto ui=ITEMS.find("equip_uniform"), ni=ITEMS.find("equip_notebook");
    if (ui!=ITEMS.end()) equip(dynamic_cast<Equipment*>(ui->second));
    if (ni!=ITEMS.end()) equip(dynamic_cast<Equipment*>(ni->second));
}

// 【考点4.1 角色管理】查看角色信息: 等级/HP/EXP/金币/学科/装备/秘宝
void Player::display() const {
    cout << string(50,'=') << endl;
    cout << "  [角色信息]" << endl;
    cout << "  姓名: " << name << "  等级: Lv." << level << endl;
    cout << "  HP: " << hp << "/" << max_hp << "  EXP: " << exp << "/" << exp_to_next << endl;
    cout << "  金币: " << gold << endl;
    cout << "  学识(ATK): " << atk() << "  抗压(DFS): " << dfs() << endl;
    cout << "  气: " << qi << "/" << max_qi << endl;
    string tn = (track=="arts")?"文科":(track=="sciences"?"理科":"未选择");
    cout << "  方向: " << tn << endl;
    cout << "  学科掌握: ";
    bool first=true;
    for (auto& [s,m] : subjects) {
        if (!first) cout << ", ";
        cout << s << "(" << m << ")";
        first=false;
    }
    cout << endl;
    auto it_c = equipment.find("clothes");
    cout << "  装备-衣服: " << (it_c!=equipment.end()&&it_c->second?it_c->second->name:"无") << endl;
    auto it_t = equipment.find("tool");
    cout << "  装备-文具: " << (it_t!=equipment.end()&&it_t->second?it_t->second->name:"无") << endl;
    if (!treasures.empty()) {
        cout << "  秘宝: ";
        for (size_t i=0; i<treasures.size(); i++) {
            if (i>0) cout << ", ";
            cout << treasures[i];
        }
        cout << endl;
    }
    cout << string(50,'=') << endl;
}

// 【考点5 数据持久化】保存功能: 将角色全部数据写入文本文件
// 使用ofstream逐行存储, 格式为key-value, 支持完整恢复
void Player::save(const string& filename) {
    ofstream f(filename);
    f<<name<<"\n"<<level<<"\n"<<exp<<"\n"<<exp_to_next<<"\n"<<gold<<"\n"
     <<max_hp<<"\n"<<hp<<"\n"<<base_atk<<"\n"<<base_dfs<<"\n"
     <<atk_mult<<"\n"<<dfs_mult<<"\n"<<exp_mult<<"\n"<<ally_atk_mult<<"\n"<<gold_mult<<"\n"
     <<(damage_true?"1":"0")<<"\n"<<(has_revive?"1":"0")<<"\n"
     <<track<<"\n"<<qi<<"\n"<<max_qi<<"\n";
    f<<subjects.size()<<"\n";
    for (auto&[s,m]:subjects) f<<s<<"\n"<<m<<"\n";
    f<<inventory.size()<<"\n";
    for (auto&[id,qty]:inventory) f<<id<<"\n"<<qty<<"\n";
    f<<card_buffs.size()<<"\n";
    for (auto& buff:card_buffs) {
        f<<buff.size()<<"\n";
        for (auto&[k,v]:buff) f<<k<<"\n"<<v<<"\n";
    }
    f<<completed_quests.size()<<"\n";
    for (auto& q:completed_quests) f<<q<<"\n";
    f<<treasures.size()<<"\n";
    for (auto& t:treasures) f<<t<<"\n";
    auto it_c=equipment.find("clothes");
    f<<(it_c!=equipment.end()&&it_c->second ? it_c->second->item_id : "none")<<"\n";
    auto it_t=equipment.find("tool");
    f<<(it_t!=equipment.end()&&it_t->second ? it_t->second->item_id : "none")<<"\n";
    // 【新增】V2扩展数据: 成长可视化追踪
    f<<"V2"<<"\n";
    f<<growth_snapshots.size()<<"\n";
    for(auto& gs:growth_snapshots)
        f<<gs.level<<" "<<gs.max_hp<<" "<<gs.atk<<" "<<gs.dfs<<" "<<gs.timestamp<<"\n";
    f<<stage_records.size()<<"\n";
    for(auto& sr:stage_records)
        f<<sr.stage_id<<"\n"<<sr.stage_name<<"\n"<<(sr.cleared?"1":"0")<<"\n"<<sr.turns_taken<<"\n"<<sr.timestamp<<"\n";
    f<<item_usage_count.size()<<"\n";
    for(auto&[id,cnt]:item_usage_count)
        f<<id<<"\n"<<cnt<<"\n";
    f<<total_battles<<"\n"<<total_wins<<"\n";
}

// 【考点5 数据持久化】加载功能: 从文本文件恢复角色全部数据
// 使用ifstream逐行读取, 与save格式对称
Player Player::load(const string& filename) {
    ifstream f(filename);
    Player p; string line;
    getline(f,p.name);
    getline(f,line); p.level=stoi(line);
    getline(f,line); p.exp=stoi(line);
    getline(f,line); p.exp_to_next=stoi(line);
    getline(f,line); p.gold=stoi(line);
    getline(f,line); p.max_hp=stoi(line);
    getline(f,line); p.hp=stoi(line);
    getline(f,line); p.base_atk=stoi(line);
    getline(f,line); p.base_dfs=stoi(line);
    getline(f,line); p.atk_mult=stod(line);
    getline(f,line); p.dfs_mult=stod(line);
    getline(f,line); p.exp_mult=stod(line);
    getline(f,line); p.ally_atk_mult=stod(line);
    getline(f,line); p.gold_mult=stod(line);
    getline(f,line); p.damage_true=(line=="1");
    getline(f,line); p.has_revive=(line=="1");
    getline(f,p.track);
    getline(f,line); p.qi=stoi(line);
    getline(f,line); p.max_qi=stoi(line);
    getline(f,line); int n_subj=stoi(line);
    for (int i=0;i<n_subj;i++) { string s,m; getline(f,s); getline(f,m); p.subjects[s]=m; }
    getline(f,line); int n_inv=stoi(line);
    for (int i=0;i<n_inv;i++) { string id; int qty; getline(f,id); getline(f,line); qty=stoi(line); p.inventory[id]=qty; }
    getline(f,line); int n_buffs=stoi(line);
    for (int i=0;i<n_buffs;i++) {
        getline(f,line); int n_kv=stoi(line);
        map<string,string> buff;
        for (int j=0;j<n_kv;j++) { string k,v; getline(f,k); getline(f,v); buff[k]=v; }
        p.card_buffs.push_back(buff);
    }
    getline(f,line); int n_cq=stoi(line);
    for (int i=0;i<n_cq;i++) { string q; getline(f,q); p.completed_quests.push_back(q); }
    getline(f,line); int n_t=stoi(line);
    for (int i=0;i<n_t;i++) { string t; getline(f,t); p.treasures.push_back(t); }
    string cid,tid; getline(f,cid); getline(f,tid);
    if (cid!="none"&&ITEMS.count(cid)) p.equipment["clothes"]=dynamic_cast<Equipment*>(ITEMS[cid]->clone());
    if (tid!="none"&&ITEMS.count(tid)) p.equipment["tool"]=dynamic_cast<Equipment*>(ITEMS[tid]->clone());
    p.rebuild_card_buff_lookup();  // 【挑战6】重建O(1)查找索引
    // 【新增】加载V2扩展数据(成长可视化)
    string v2line;
    if(getline(f,v2line) && v2line=="V2"){
        getline(f,line); int n_gs=stoi(line);
        for(int i=0;i<n_gs;i++){
            getline(f,line);
            istringstream gss(line);
            GrowthSnapshot gs;
            gss>>gs.level>>gs.max_hp>>gs.atk>>gs.dfs;
            getline(gss,gs.timestamp);
            if(!gs.timestamp.empty()&&gs.timestamp[0]==' ') gs.timestamp=gs.timestamp.substr(1);
            p.growth_snapshots.push_back(gs);
        }
        getline(f,line); int n_sr=stoi(line);
        for(int i=0;i<n_sr;i++){
            StageRecord sr;
            getline(f,sr.stage_id); getline(f,sr.stage_name);
            getline(f,line); sr.cleared=(line=="1");
            getline(f,line); sr.turns_taken=stoi(line);
            getline(f,sr.timestamp);
            p.stage_records.push_back(sr);
        }
        getline(f,line); int n_iu=stoi(line);
        for(int i=0;i<n_iu;i++){
            string id; int cnt;
            getline(f,id); getline(f,line); cnt=stoi(line);
            p.item_usage_count[id]=cnt;
        }
        getline(f,line); p.total_battles=stoi(line);
        getline(f,line); p.total_wins=stoi(line);
    }
    return p;
}

// ============================================
// Card Subclass implementations
// 【考点1.2 继承】10种卡牌均继承自Card基类
// 【考点1.3 多态】每个子类重写apply()实现不同的战斗效果
// ============================================
Card1SubjectUpgrade::Card1SubjectUpgrade() : Card("card1","","若对该学科不了解则变为掌握","subject_upgrade") {}
pair<bool,string> Card1SubjectUpgrade::apply(Player& player, BattleContext*) {
    for (auto& [subj, mastery] : player.subjects)
        if (mastery == "了解") { player.subjects[subj]="精通"; return {true,"[卡牌1] "+subj+"从「了解」提升为「精通」！"}; }
    return {false,"[卡牌1] 没有可升级的学科。"};
}

Card2PercentTrueDamage::Card2PercentTrueDamage() : Card("card2","","每次伤害附加对方血量5%的真实伤害","percent_true_damage",5) {}
pair<bool,string> Card2PercentTrueDamage::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","percent_true_damage"},{"value",to_string(effect_value)}});
    return {true,"[卡牌2] 附加对方血量5%的真实伤害"};
}

Card3LifeStealPct::Card3LifeStealPct() : Card("card3","","每次伤害附带12％的吸血","lifesteal_pct",12) {}
pair<bool,string> Card3LifeStealPct::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","lifesteal_pct"},{"value",to_string(effect_value)}});
    return {true,"[卡牌3] 每次伤害附带12%吸血"};
}

Card4DamageReduction::Card4DamageReduction() : Card("card4","","获取20%伤害减免","damage_reduction",20) {}
pair<bool,string> Card4DamageReduction::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","damage_reduction"},{"value",to_string(effect_value)}});
    return {true,"[卡牌4] 获取20%伤害减免"};
}

Card5EnergyReturn::Card5EnergyReturn() : Card("card5","","大招使用后返回50%的能量(若为小数则向上取整)","energy_return",50) {}
pair<bool,string> Card5EnergyReturn::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","energy_return"},{"value",to_string(effect_value)}});
    return {true,"[卡牌5] 大招返还50%能量"};
}

Card6DefendImmune::Card6DefendImmune() : Card("card6","","防御时免疫当次受到的伤害","defend_immune") {}
pair<bool,string> Card6DefendImmune::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","defend_immune"},{"value","1"}});
    return {true,"[卡牌6] 防御时免疫当次伤害"};
}

Card7EnergyEfficiency::Card7EnergyEfficiency() : Card("card7","","大招充能效率增加100%","energy_efficiency",100) {}
pair<bool,string> Card7EnergyEfficiency::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","energy_efficiency"},{"value",to_string(effect_value)}});
    return {true,"[卡牌7] 大招充能效率+100%"};
}

Card8SurviveCharge::Card8SurviveCharge() : Card("card8","","受到致命伤害时血量变为1，下回合获得无敌，充满大招","survive_charge") {}
pair<bool,string> Card8SurviveCharge::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","survive_charge"},{"value","1"}});
    return {true,"[卡牌8] 致命伤害HP=1，无敌1回合，充满大招"};
}

Card9UltimateDebuff::Card9UltimateDebuff() : Card("card9","","使用大招是，先给对面挂上破防(防御力降低25%)，再造成伤害","ultimate_debuff",25) {}
pair<bool,string> Card9UltimateDebuff::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","ultimate_debuff"},{"value",to_string(effect_value)}});
    return {true,"[卡牌9] 大招附带破防25%"};
}

Card10ReflectDamage::Card10ReflectDamage() : Card("card10","","你是一个香香软软糯糯的小猫娘，无敌且反弹100%的伤害","reflect_damage",100) {}
pair<bool,string> Card10ReflectDamage::apply(Player& player, BattleContext*) {
    player.card_buffs.push_back({{"type","reflect_damage"},{"value",to_string(effect_value)}});
    return {true,"[卡牌10] 无敌+反弹100%伤害"};
}

// ============================================
// Treasure Subclass implementations
// 【考点1.2 继承】6种藏品均继承自Treasure基类
// 【考点1.3 多态】每个子类重写apply()和remove()实现不同效果
// ============================================
Treasure1DfsBoost::Treasure1DfsBoost() : Treasure("treasure1","Past<1>对于清北的渴望","从学习中获取知识，日复一日的坚持，抗压力增加50%","dfs_boost",50) {}
string Treasure1DfsBoost::apply(Player& player) { player.dfs_mult+=effect_value/100.0; return "[藏品1] 抗压+50%"; }
void Treasure1DfsBoost::remove(Player& player) { player.dfs_mult-=effect_value/100.0; }

Treasure2ExpGoldBoost::Treasure2ExpGoldBoost() : Treasure("treasure2","Past<2>效率","本关卡获得的经验与金币增加100%","exp_gold_boost",100) {}
string Treasure2ExpGoldBoost::apply(Player& player) {
    player.exp_mult+=effect_value/100.0; player.gold_mult+=effect_value/100.0;
    return "[藏品2] 经验+100%, 金币+100%";
}
void Treasure2ExpGoldBoost::remove(Player& player) { player.exp_mult-=effect_value/100.0; player.gold_mult-=effect_value/100.0; }

Treasure3ReviveTruePath::Treasure3ReviveTruePath() : Treasure("treasure3","Past<3>变革已至","当学习时屡屡溃败时不妨变革以下方法，或许可以找到新的出路，血量归零后复活。学识增加20%，所有伤害均变为真实伤害","revive_true_path") {}
string Treasure3ReviveTruePath::apply(Player& player) {
    player.atk_mult+=0.2; player.damage_true=true; player.has_revive=true;
    return "[藏品3] 学识+20%，伤害变为真实伤害，获得复活机会！";
}
void Treasure3ReviveTruePath::remove(Player& player) { player.atk_mult-=0.2; player.damage_true=false; player.has_revive=false; }

Treasure4AllyBoost::Treasure4AllyBoost() : Treasure("treasure4","Past<4>春日影的余响","光は やさしく連れ立つよ，我方士气高昂，伤害增加20%","ally_atk_boost",20) {}
string Treasure4AllyBoost::apply(Player& player) { player.ally_atk_mult+=effect_value/100.0; return "[藏品4] 我方伤害+20%"; }
void Treasure4AllyBoost::remove(Player& player) { player.ally_atk_mult-=effect_value/100.0; }

Treasure5RandomNotebook::Treasure5RandomNotebook() : Treasure("treasure5","Past<5>学长的遗产","获得装备学长笔记的效果(随机一个学科的笔记)，我方对该学科变为擅长","random_notebook") {}
string Treasure5RandomNotebook::apply(Player& player) {
    string subj = rand_choice(ALL_SUBJECTS);
    player.subjects[subj] = "精通";
    return "[藏品5] 随机获得"+subj+"笔记效果！"+subj+"变为「精通」！";
}
void Treasure5RandomNotebook::remove(Player&) {}

Treasure6YachiyoSummon::Treasure6YachiyoSummon() : Treasure("treasure6","Past<6>月见八千代","召唤月见八千代(血量9999999，学识750，抗压力799，大招:跨越千年与你相遇（酒寄彩叶），大招能量为3点，普攻与受攻击均回复1点能量)，对方攻击普攻及无群体AOE伤害的大招为角色与月见八千代由AI控制智能选择我方角色攻击，我方召唤师死去，月见八千代消失","summon") {}
string Treasure6YachiyoSummon::apply(Player& player) {
    player.card_buffs.push_back({{"type","summon_yachiyo"},{"used","0"}});
    return "[藏品6] 可以在战斗中召唤月见八千代！";
}
void Treasure6YachiyoSummon::remove(Player& player) {
    vector<map<string,string>> nb;
    for (auto& b : player.card_buffs) if (!b.count("type")||b["type"]!="summon_yachiyo") nb.push_back(b);
    player.card_buffs=nb;
}

// ============================================
// 第五部分: 战斗系统 (BattleSystem)
// 负责回合制战斗的完整流程: 玩家回合→敌人回合→回合结束
// 【挑战7:AI】集成AIDecisionEngine实现智能敌方决策
// 【挑战2:多线程】集成回合计时器
// ============================================

// ============================================
// 【挑战7:AI】AIDecisionEngine 智能决策系统
// 基于加权评分决策树:
//   - 根据玩家HP%、气能量、敌方HP%、buff状态综合评估
//   - 如果玩家有反弹buff → 优先治疗/防御，避免攻击反弹
//   - 如果玩家HP低(<30%) → 优先使用高伤害技能
//   - 如果敌方HP低(<30%) → 优先治疗自己
//   - 如果八千代在场 → 决策是否攻击召唤物
// 支持大模型API扩展(需curl, 默认禁用)
// ============================================
AIDecisionEngine::Action AIDecisionEngine::decide(const BattleSnapshot& snap) {
    // LLM模式: 调用大模型API获取决策
    if(llm_config.enabled) {
        string llm_resp = call_llm_api(llm_config, snap);
        if(!llm_resp.empty()) {
            decision_reason = "[LLM决策] " + llm_resp;
            if(llm_resp.find("skill")!=string::npos || llm_resp.find("SKILL")!=string::npos)
                return SKILL;
            if(llm_resp.find("defend")!=string::npos || llm_resp.find("DEFEND")!=string::npos)
                return DEFEND;
            if(llm_resp.find("heal")!=string::npos || llm_resp.find("HEAL")!=string::npos)
                return HEAL;
            if(llm_resp.find("summon")!=string::npos || llm_resp.find("YACHIYO")!=string::npos)
                return TARGET_SUMMON;
            return ATTACK;
        }
        // LLM调用失败, 回退到本地AI
        decision_reason = "[LLM失败,切换本地AI] ";
    }
    
    // 本地智能AI: 加权评分决策树
    vector<pair<Action,int>> scores;
    
    if(snap.enemy_is_boss) {
        if(snap.enemy_has_aoe) scores.push_back({SKILL, score_skill(snap)});
        if(snap.enemy_has_heal) scores.push_back({HEAL, score_heal(snap)});
        if(snap.enemy_has_td) scores.push_back({TRUE_DMG, score_heal(snap)+10});
        scores.push_back({DEFEND, score_defend(snap)});
    }
    
    if(snap.yachiyo_present)
        scores.push_back({TARGET_SUMMON, score_target_summon(snap)});
    scores.push_back({ATTACK_PLAYER, score_attack(snap)});
    
    // 【挑战6:STL高级应用】使用nth_element按评分降序,找最高分决策
    // 若scores.size()>1, 取Top1
    if(scores.size()>1) {
        nth_element(scores.begin(), scores.begin(), scores.end(),
            [](const pair<Action,int>& a, const pair<Action,int>& b) {
                return a.second > b.second;
            });
    }
    
    decision_reason += "[本地AI评分决策]";
    return scores[0].first;
}

int AIDecisionEngine::score_attack(const BattleSnapshot& snap) {
    int score = 50;
    // 玩家HP越低, 攻击欲望越强
    if(snap.player_hp_pct < 30) score += 30;
    else if(snap.player_hp_pct < 50) score += 15;
    // 玩家如果免疫或反弹, 攻击降分
    if(snap.player_has_reflect || snap.player_immuned) score -= 40;
    // Boss额外加分
    if(snap.enemy_is_boss) score += 10;
    return score;
}

int AIDecisionEngine::score_skill(const BattleSnapshot& snap) {
    int score = 60;
    if(snap.player_hp_pct < 50) score += 25;
    if(snap.player_has_reflect) score -= 50;  // AOE也会反弹
    if(snap.turn_count > 5) score += 15;       // 后期更倾向放大招
    return score;
}

int AIDecisionEngine::score_defend(const BattleSnapshot& snap) {
    int score = 30;
    if(snap.player_has_reflect) score += 30;
    if(snap.player_qi >= 6) score += 25;       // 玩家快满气, 防御减伤
    if(snap.enemy_hp_pct < 30) score += 10;    // 残血时防御保命
    return score;
}

int AIDecisionEngine::score_heal(const BattleSnapshot& snap) {
    int score = 40;
    if(snap.enemy_hp_pct < 30) score += 40;    // 残血优先治疗
    if(snap.enemy_hp_pct < 20) score += 20;
    if(snap.player_hp_pct < 30) score -= 20;   // 玩家残血时少治疗,多输出
    return score;
}

int AIDecisionEngine::score_target_summon(const BattleSnapshot& snap) {
    int score = 45;
    if(snap.yachiyo_hp_pct < 50) score += 20;  // 八千代残血优先打
    if(snap.player_hp_pct < 30) score -= 15;   // 玩家快死,优先打玩家
    if(snap.enemy_is_boss) score -= 10;        // Boss更倾向打玩家
    return score;
}

void AIDecisionEngine::enable_llm(const LLMConfig& cfg) {
    llm_config = cfg;
    cout << "[AI系统] 大模型接口已启用 (" << cfg.model << ")" << endl;
}

void AIDecisionEngine::disable_llm() {
    llm_config.enabled = false;
    cout << "[AI系统] 切换为本地智能AI模式" << endl;
}

// 【挑战7扩展】LLM API调用接口(需编译时定义 USE_LLM 并链接libcurl)
string AIDecisionEngine::call_llm_api(const LLMConfig& cfg, const BattleSnapshot& snap) {
    (void)cfg; (void)snap;
    // 实际项目中可在此集成 libcurl 调用 DeepSeek/OpenAI API:
    // 构造JSON请求 → POST → 解析响应 → 返回决策指令
    // 示例:
    //   curl -X POST {cfg.api_url} -H "Authorization: Bearer {cfg.api_key}"
    //        -d '{"model":"{cfg.model}","messages":[{"role":"system",
    //        "content":"你是校园RPG的AI对手决策系统..."}],...}'
    // 由于编译环境不确定, 此处保留接口, 默认返回空字符串
    // 如需启用: 使用 -DUSE_LLM -lcurl 编译
    return "";
}

// ============================================
// 【挑战2:多线程】战斗计时系统
// start_battle_timer: 记录当前时间戳作为回合起点
// is_battle_timer_expired: 检查是否超过n秒限制
// 用于实现回合时间限制(超时自动防御)
// ============================================
void BattleSystem::start_battle_timer() {
    battle_timer_start = chrono::steady_clock::now();
}

bool BattleSystem::is_battle_timer_expired(int seconds) {
    auto now = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::seconds>(now - battle_timer_start).count();
    return elapsed >= seconds;
}

// ============================================
// 【挑战7:AI】构建战斗状态快照
// 从当前战斗状态提取关键信息供AI决策使用
// ============================================
AIDecisionEngine::BattleSnapshot BattleSystem::build_ai_snapshot(const Enemy& e) {
    AIDecisionEngine::BattleSnapshot snap;
    snap.player_hp_pct = player->max_hp > 0 ? player->hp * 100 / player->max_hp : 0;
    snap.player_qi = player->qi;
    snap.player_has_reflect = player->has_card_buff_type("reflect_damage");
    snap.player_immuned = player->immune_turns > 0;
    snap.enemy_hp_pct = e.max_hp > 0 ? e.hp * 100 / e.max_hp : 0;
    snap.enemy_is_boss = e.is_boss;
    snap.enemy_has_aoe = false;
    snap.enemy_has_heal = false;
    snap.enemy_has_td = false;
    for(auto& sk : e.skills) {
        if(sk == "aoe_attack") snap.enemy_has_aoe = true;
        if(sk == "heal_self") snap.enemy_has_heal = true;
        if(sk == "true_damage") snap.enemy_has_td = true;
    }
    snap.yachiyo_present = ctx && ctx->has_yachiyo();
    snap.yachiyo_hp_pct = 0;
    if(snap.yachiyo_present && ctx->yachiyo.count("max_hp") && ctx->yachiyo["max_hp"] > 0)
        snap.yachiyo_hp_pct = ctx->yachiyo["hp"] * 100 / ctx->yachiyo["max_hp"];
    snap.turn_count = ctx ? ctx->turn : 0;
    return snap;
}

// ============================================
BattleSystem::BattleSystem(Player* p) : player(p), ctx(nullptr), is_player_turn(true) {}
BattleSystem::~BattleSystem() { delete ctx; }

// ============================================
// has_energy_efficiency: 检测是否有"充能效率翻倍"卡牌buff
// 用途: 决定技能返还能量是否翻倍
// ============================================
bool BattleSystem::has_energy_efficiency() {
    for (auto& b : player->card_buffs)
        if (b.count("type") && b["type"]=="energy_efficiency") return true;
    return false;
}

// ============================================
// get_energy_refund: 计算大招使用后返还的能量值
// 算法: 基础返还(cost × 返还百分比) × 充能效率(若有则×2), 向上取整
// ============================================
int BattleSystem::get_energy_refund(int cost) {
    for (auto& b : player->card_buffs)
        if (b.count("type") && b["type"]=="energy_return") {
            int pct = stoi(b["value"]);
            if (has_energy_efficiency()) pct*=2;
            return (int)ceil(cost*pct/100.0);
        }
    return 0;
}

// ============================================
// select_target: 显示存活敌人列表, 让玩家选择攻击目标
// 单敌人: 自动选择; 多敌人: 编号选择
// 返回: 目标索引(-1为取消/无效)
// ============================================
int BattleSystem::select_target() {
    vector<int> alive;
    for (size_t i=0;i<enemies.size();i++) if (enemies[i].is_alive()) alive.push_back((int)i);
    if (alive.empty()) return -1;
    if (alive.size()==1) return alive[0];
    cout<<"\n选择目标:"<<endl;
    for (size_t i=0;i<enemies.size();i++)
        if (enemies[i].is_alive())
            cout<<"  ["<<(i+1)<<"] "<<enemies[i].name<<" (HP: "<<enemies[i].hp<<"/"<<enemies[i].max_hp<<")"<<endl;
    string s; cout<<"选择目标: "; getline(cin,s);
    try { int idx=stoi(s)-1; if(idx>=0&&(size_t)idx<enemies.size()&&enemies[idx].is_alive()) return idx; }
    catch(...){}
    return -1;
}

// ============================================
// choose_attack_subject: 让玩家选择进攻学科
// 精通学科标记★, 非精通标记○
// ============================================
string BattleSystem::choose_attack_subject() {
    vector<string> avail;
    for (auto&[s,m]:player->subjects) avail.push_back(s);
    cout<<"\n选择攻击学科:"<<endl;
    for (size_t i=0;i<avail.size();i++) {
        string mi = player->subjects[avail[i]]=="精通"?"★":"○";
        cout<<"  ["<<(i+1)<<"] "<<avail[i]<<" "<<mi<<endl;
    }
    string s; cout<<"选择学科: "; getline(cin,s);
    try { int idx=stoi(s)-1; if(idx>=0&&(size_t)idx<avail.size()) return avail[idx]; }
    catch(...){}
    return "";
}

// ============================================
// calc_damage: 伤害计算核心函数
// 公式:
//   普通攻击: max(1, ATK - DFS) × 学科克制倍率 × 增伤倍率
//   技能攻击: (ATK × 倍率) / max(1, DFS) × 克制倍率
//   真实伤害: 跳过DFS计算, 直接应用ATK
// 学科克制:
//   精通→2.0倍伤害 / 了解→0.5倍 / 未掌握→1.0倍
// ============================================
int BattleSystem::calc_damage(Enemy* attacker, const string& subject, bool is_skill, double multiplier) {
    int atk_val = player->atk();
    int dfs_val = attacker->dfs_val;
    int damage;
    if (is_skill) {
        if (player->damage_true) damage = (int)(atk_val * multiplier);
        else damage = (int)(atk_val*multiplier/(max)(1,dfs_val));
    } else {
        if (player->damage_true) damage = atk_val;
        else damage = (max)(1, atk_val-dfs_val);
    }

    int mastery = player->get_subject_mastery(subject);
    if (mastery==1) damage=(int)(damage*0.5);
    else if (mastery==2) damage=(int)(damage*2.0);

    if (ctx && ctx->has_yachiyo()) {
        int aoe=(int)(ctx->yachiyo["atk"]*0.7);
        cout<<"月见八千代发动AOE，对所有敌人造成"<<aoe<<"点伤害！"<<endl;
        for (auto& e : enemies) if (e.is_alive()) apply_damage(&e,aoe,nullptr,false);
    }

    damage = apply_ultimate_debuff(damage);
    damage = (int)(damage*player->ally_atk_mult);
    if (player->damage_true) return damage;
    return damage;
}

int BattleSystem::apply_ultimate_debuff(int damage) {
    return damage;
}

// ============================================
// apply_damage: 将伤害实际应用到目标
// 处理机制(按优先级):
//   1. 卡牌10反弹: 无敌+100%反弹, 跳过所有伤害
//   2. 卡牌4伤害减免: 伤害×(100-减免%)%
//   3. 护盾吸收: 优先消耗护盾值
//   4. 卡牌3吸血: 伤害×12%转化为HP恢复
//   5. 卡牌2真实伤害: 附加目标5%最大HP的真实伤害
//   6. 星辰笔: 附加固定元素伤害
//   7. 笔芯效果: 附加5%最大HP伤害
//   (以上4-7项可叠加触发)
// ============================================
void BattleSystem::apply_damage(Enemy* target, int damage, Enemy* attacker, bool is_enemy_damaging) {
    int actual = damage;
    if (is_enemy_damaging) {
        for (auto& b : player->card_buffs)
            if (b.count("type")&&b["type"]=="reflect_damage") {
                cout<<"无敌！反弹了"<<actual<<"点伤害！"<<endl;
                for (auto& e : enemies) if (e.is_alive()) { e.hp=max(0,e.hp-actual); cout<<"  反弹伤害给"<<e.name<<"！"<<endl; }
                return;
            }
        for (auto& b : player->card_buffs)
            if (b.count("type")&&b["type"]=="damage_reduction") {
                int pct=stoi(b["value"]);
                if (has_energy_efficiency()) pct*=2;
                actual=actual*(100-pct)/100;
                if (actual<=0) { cout<<"伤害已减免！"<<endl; return; }
            }
    }
    if (target->shield>0) {
        if (target->shield>=actual) { target->shield-=actual; actual=0; }
        else { actual-=target->shield; target->shield=0; }
    }
    if (actual>0) {
        target->hp=(max)(0,target->hp-actual);
        if (target->hp<=0) {
            for (auto& b : target->card_buffs)
                if (b.count("type")&&b["type"]=="survive"&&!target->survive_used) {
                    target->hp=1; target->immune_turns=1; target->survive_used=true;
                    cout<<"[坚韧意志] 锁定HP=1，下回合免疫！"<<endl; break;
                }
        }
    }
    if (!is_enemy_damaging && target->is_alive()) {
        for (auto& b : player->card_buffs)
            if (b.count("type")&&b["type"]=="lifesteal_pct") {
                int steal=(int)(actual*stoi(b["value"])/100.0);
                if (has_energy_efficiency()) steal*=2;
                player->hp=min(player->hp+steal,player->max_hp);
                cout<<"[卡牌3] 吸取"<<steal<<"点HP！"<<endl; break;
            }
        for (auto& b : player->card_buffs)
            if (b.count("type")&&b["type"]=="percent_true_damage") {
                int pct=(int)(target->max_hp*stoi(b["value"])/100.0);
                if (has_energy_efficiency()) pct*=2;
                target->hp=(max)(0,target->hp-pct);
                cout<<"[精准打击] 附加"<<pct<<"点真实伤害！"<<endl; break;
            }
        for (auto& b : player->card_buffs)
            if (b.count("type")&&b["type"]=="star_pen") {
                int bonus=stoi(b["value"]);
                if (has_energy_efficiency()) bonus*=2;
                target->hp=(max)(0,target->hp-bonus);
                cout<<"[星辰笔] 追加"<<bonus<<"点元素伤害！"<<endl; break;
            }
        for (auto& b : player->card_buffs)
            if (b.count("type")&&b["type"]=="pen_core") {
                int pct=stoi(b["value"]);
                int pen_dmg=(int)(target->max_hp*pct/100.0);
                target->hp=(max)(0,target->hp-pen_dmg);
                cout<<"[笔芯] 追加"<<pen_dmg<<"点百分比伤害！"<<endl; break;
            }
        // 【挑战6:STL高级应用】学科伤害统计
        if(!attacker && !is_enemy_damaging)
            player->subject_stats[target->subject_tag]++;
    }
    if (!is_enemy_damaging && stage_effects.count("n5_enemy_bonus") && stage_effects.at("n5_enemy_bonus")) {
        int before=player->hp;
        player->hp=(min)(player->max_hp,player->hp+(int)(actual*0.12));
        cout<<"[N5效果] 附加10点真实伤害，吸血"<<(player->hp-before)<<"点HP！"<<endl;
    }
}

// ============================================
// Challenge 2: 多线程技术 - AutoSaveSystem 实现
// worker_loop: 后台线程主循环, 每interval秒触发一次保存
// 使用mutex保护文件写入, atomic控制运行状态
// ============================================
// Challenge 2: 多线程技术 - AutoSaveSystem 实现
// worker_loop: 后台线程主循环, 每interval秒触发一次保存
// 使用mutex保护文件写入, atomic控制运行状态
// ============================================
void AutoSaveSystem::start() {
    if(running.load()) return;
    running.store(true);
    worker = thread(&AutoSaveSystem::worker_loop, this);
    cout << "[自动存档] 后台自动保存已启动 (间隔" << interval.count() << "秒)" << endl;
}

void AutoSaveSystem::stop() {
    running.store(false);
    save_needed.store(false);
    if(worker.joinable()) worker.join();
}

void AutoSaveSystem::do_save() {
    lock_guard<mutex> lock(mtx);
#ifdef _WIN32
    system("mkdir saves 2>nul");
#else
    system("mkdir -p saves");
#endif
    player->save("saves/save.json");
    cout << "[自动存档] 进度已保存！" << endl;
}

void AutoSaveSystem::worker_loop() {
    while(running.load()) {
        // 间歇检查: 每1秒检查一次
        for(int i = 0; i < interval.count() && running.load(); i++) {
            // 【挑战2:多线程】同时支持save_needed标志的主动触发存档
            if(save_needed.load()) {
                save_needed.store(false);
                break;
            }
            this_thread::sleep_for(chrono::seconds(1));
        }
        if(running.load()) do_save();
    }
}

// ============================================
// Challenge 2: 多线程技术 - 全局自动存档入口函数
// 可用于创建独立的后台线程(不与AutoSaveSystem绑定)
// ============================================
void auto_save_worker(Player* player, atomic<bool>* running, mutex* mtx, chrono::seconds interval) {
    while(running->load()) {
        for(int i = 0; i < interval.count() && running->load(); i++)
            this_thread::sleep_for(chrono::seconds(1));
        if(running->load()) {
            lock_guard<mutex> lock(*mtx);
#ifdef _WIN32
            system("mkdir saves 2>nul");
#else
            system("mkdir -p saves");
#endif
            player->save("saves/save.json");
        }
    }
}

// ============================================
// player_normal_attack: 玩家普通攻击
// 流程: 选择目标 → 选择学科 → 计算伤害 → 应用伤害 → 获得1气(或2气)
// 特殊: 披星戴月状态下变为AOE真实伤害
// ============================================
void BattleSystem::player_normal_attack() {
    if (ctx && ctx->has_pi_ya_turns()) {
        player_normal_attack_aoe();
        return;
    }
    int idx=select_target(); if(idx<0) return;
    string subj=choose_attack_subject(); if(subj.empty()) return;
    int dmg=calc_damage(&enemies[idx],subj);
    apply_damage(&enemies[idx],dmg,nullptr,false);
    cout<<"\n你使用"<<subj<<"对"<<enemies[idx].name<<"造成了"<<dmg<<"点伤害！"<<endl;
    int qi_gain=has_energy_efficiency()?4:2;
    player->gain_qi(qi_gain);
    if(qi_gain>2) cout<<"[卡牌7] 充能效率翻倍！";
    cout<<"普攻回复"<<qi_gain<<"点气！当前气: "<<player->qi<<"/"<<player->max_qi<<endl;
}

// ============================================
// player_skill_attack: 玩家技能攻击(消耗气)
// 只能使用"精通"学科对应的技能(消耗3气, 2.0倍率)
// ============================================
void BattleSystem::player_skill_attack() {
    auto skills=player->get_available_skills();
    if(skills.empty()){cout<<"你还没有精通任何学科！"<<endl;return;}
    cout<<"\n可用技能:"<<endl;
    for(size_t i=0;i<skills.size();i++)
        cout<<"  ["<<(i+1)<<"] "<<skills[i]["name"]<<" ("<<skills[i]["desc"]<<") 消耗:"<<skills[i]["cost"]<<"气"<<endl;
    string s; cout<<"选择技能(0=取消): "; getline(cin,s);
    try {
        int ch=stoi(s)-1;
        if(ch==-1) return;
        if(ch>=0&&(size_t)ch<skills.size()) {
            int cost=stoi(skills[ch]["cost"]);
            if(!player->consume_qi(cost)){cout<<"气不足！"<<endl;return;}
            int idx=select_target(); if(idx<0) return;
            int dmg=calc_damage(&enemies[idx],skills[ch]["subject"],true,stod(skills[ch]["multiplier"]));
            apply_damage(&enemies[idx],dmg,nullptr,false);
            int refund=get_energy_refund(cost);
            if(refund>0){player->qi+=refund; cout<<"[节能思维] 返还"<<refund<<"点气！"<<endl;}
            cout<<"\n你使用"<<skills[ch]["name"]<<"对"<<enemies[idx].name<<"造成了"<<dmg<<"点伤害！"<<endl;
        }
    }catch(...){cout<<"请输入有效数字！"<<endl;}
}

void BattleSystem::player_use_item() {
    if(stage_effects.count("no_potion")&&stage_effects.at("no_potion")){cout<<"本关卡禁止使用药品！"<<endl;return;}
    vector<pair<string,Item*>> potions;
    for(auto&[id,qty]:player->inventory)
        if(qty>0&&ITEMS.count(id)&&ITEMS[id]->item_type=="potion") potions.push_back({id,ITEMS[id]});
    if(potions.empty()){cout<<"没有可用的药品！"<<endl;return;}
    cout<<"\n可用药品:"<<endl;
    for(size_t i=0;i<potions.size();i++)
        cout<<"  ["<<(i+1)<<"] "<<potions[i].second->name<<" x"<<player->inventory[potions[i].first]<<" - "<<potions[i].second->desc<<endl;
    string s; cout<<"选择药品(0=取消): "; getline(cin,s);
    try{int ch=stoi(s)-1; if(ch==-1)return; if(ch>=0&&(size_t)ch<potions.size()){
        auto[succ,msg]=potions[ch].second->use(*player); if(succ){player->remove_item(potions[ch].first); player->record_item_usage(potions[ch].first);} cout<<"\n"<<msg<<endl;
    }}catch(...){cout<<"请输入有效数字！"<<endl;}
}

void BattleSystem::player_use_card() {
    if(player->card_buffs.empty()){cout<<"没有可用的卡牌效果。"<<endl;return;}
    cout<<"\n当前卡牌效果(自动生效):"<<endl;
    for(size_t i=0;i<player->card_buffs.size();i++)
        cout<<"  ["<<(i+1)<<"] "<<(player->card_buffs[i].count("name")?player->card_buffs[i]["name"]:"未知")<<" - "<<(player->card_buffs[i].count("desc")?player->card_buffs[i]["desc"]:player->card_buffs[i]["type"])<<endl;
}

void BattleSystem::player_normal_attack_aoe() {
    string subj=choose_attack_subject(); if(subj.empty()) return;
    double orig_mult=player->atk_mult;
    bool orig_dt=player->damage_true;
    player->atk_mult+=3.0;
    player->damage_true=true;
    cout<<"\n[披星戴月] 学识提升300%，普攻变为真实伤害AOE！"<<endl;
    for(size_t i=0;i<enemies.size();i++) {
        if(!enemies[i].is_alive()) continue;
        int dmg=calc_damage(&enemies[i],subj);
        apply_damage(&enemies[i],dmg,nullptr,false);
        cout<<"  对"<<enemies[i].name<<"造成了"<<dmg<<"点真实伤害！"<<endl;
    }
    player->gain_qi(1);
    cout<<"普攻回复1点气！当前气: "<<player->qi<<"/"<<player->max_qi<<endl;
    player->atk_mult=orig_mult; player->damage_true=orig_dt;
    int qi_gain=has_energy_efficiency()?2:1;
    player->gain_qi(qi_gain);
    if(qi_gain>1) cout<<"[卡牌7] 充能效率翻倍！";
    cout<<"普攻回复"<<qi_gain<<"点气！当前气: "<<player->qi<<"/"<<player->max_qi<<endl;
}

bool BattleSystem::player_use_ultimate() {
    cout<<"\n--- 角色大招 ---"<<endl;
    cout<<"  1. 天动万象 - AOE伤害，提高血量与抗压2回合 (消耗6气)"<<endl;
    cout<<"  2. 笔破十方 - 高额单体伤害+恢复自身学识*100%的血 (消耗6气)"<<endl;
    cout<<"  3. 披星戴月 - 强化普攻3回合，学识+300%，真实伤害AOE (消耗6气)"<<endl;
    cout<<"  0. 取消"<<endl;
    string s; cout<<"选择大招: "; getline(cin,s);
    if(s=="1"){
        if(!player->consume_qi(6)){cout<<"气不足！"<<endl;return false;}
        ultimate_tian_dong_wan_xiang();
        return true;
    }else if(s=="2"){
        if(!player->consume_qi(6)){cout<<"气不足！"<<endl;return false;}
        ultimate_bi_po_shi_fang();
        return true;
    }else if(s=="3"){
        if(!player->consume_qi(6)){cout<<"气不足！"<<endl;return false;}
        ultimate_pi_xing_dai_yue();
        return true;
    }
    return false;
}

void BattleSystem::ultimate_tian_dong_wan_xiang() {
    cout<<"\n[天动万象] 万象俱寂，天动地摇！"<<endl;
    for(auto& b:player->card_buffs) if(b.count("type")&&b["type"]=="ultimate_debuff"){
        int pct=stoi(b["value"]);
        for(auto& e:enemies) if(e.is_alive()){ e.dfs_val=e.dfs_val*(100-pct)/100; cout<<"[卡牌9] "<<e.name<<"防御降低"<<pct<<"%！"<<endl; }
        break;
    }
    string subj=rand_choice(ALL_SUBJECTS);
    for(size_t i=0;i<enemies.size();i++) {
        if(!enemies[i].is_alive()) continue;
        int dmg=calc_damage(&enemies[i],subj,true,2.5);
        apply_damage(&enemies[i],dmg,nullptr,false);
        cout<<"  对"<<enemies[i].name<<"造成了"<<dmg<<"点AOE伤害！"<<endl;
    }
    int hp_buff=30, dfs_buff=15;
    player->max_hp+=hp_buff; player->hp+=hp_buff;
    player->base_dfs+=dfs_buff;
    ctx->tian_xian_turns=2;
    ctx->tian_xian_hp_buff=hp_buff;
    ctx->tian_xian_dfs_buff=dfs_buff;
    ctx->tian_xian_orig_max_hp=player->max_hp-hp_buff;
    cout<<"血量+"<<hp_buff<<"，抗压+"<<dfs_buff<<"，持续2回合！"<<endl;
}

void BattleSystem::ultimate_bi_po_shi_fang() {
    int idx=select_target(); if(idx<0){player->gain_qi(6);return;}
    cout<<"\n[笔破十方] 一笔破十方！"<<endl;
    for(auto& b:player->card_buffs) if(b.count("type")&&b["type"]=="ultimate_debuff"){
        int pct=stoi(b["value"]);
        enemies[idx].dfs_val=enemies[idx].dfs_val*(100-pct)/100;
        cout<<"[卡牌9] "<<enemies[idx].name<<"防御力降低"<<pct<<"%！"<<endl;
        break;
    }
    string subj=rand_choice(ALL_SUBJECTS);
    int dmg=calc_damage(&enemies[idx],subj,true,3.5);
    apply_damage(&enemies[idx],dmg,nullptr,false);
    cout<<"对"<<enemies[idx].name<<"造成了"<<dmg<<"点高额伤害！"<<endl;
    int heal=player->atk();
    int before=player->hp;
    player->hp=min(player->hp+heal,player->max_hp);
    cout<<"吸收了"<<(player->hp-before)<<"点生命值！"<<endl;
}

void BattleSystem::ultimate_pi_xing_dai_yue() {
    cout<<"\n[披星戴月] 披星戴月，所向披靡！"<<endl;
    for(auto& b:player->card_buffs) if(b.count("type")&&b["type"]=="ultimate_debuff"){
        int pct=stoi(b["value"]);
        for(auto& e:enemies) if(e.is_alive()){ e.dfs_val=e.dfs_val*(100-pct)/100; cout<<"[卡牌9] "<<e.name<<"防御降低"<<pct<<"%！"<<endl; }
        break;
    }
    ctx->pi_ya_turns=3;
    cout<<"普攻强化3回合：学识+300%，真实伤害AOE！"<<endl;
}

void BattleSystem::process_buff_expiry() {
    if(ctx->tian_xian_turns>0){
        ctx->tian_xian_turns--;
        if(ctx->tian_xian_turns==0){
            player->max_hp-=ctx->tian_xian_hp_buff;
            player->hp=min(player->hp,player->max_hp);
            player->base_dfs-=ctx->tian_xian_dfs_buff;
            cout<<"[天动万象] buff已结束。"<<endl;
            if(player->hp<ctx->tian_xian_orig_max_hp){
                cout<<"[天动万象] 血量低于原血量，触发追加伤害！"<<endl;
                string subj=rand_choice(ALL_SUBJECTS);
                for(size_t i=0;i<enemies.size();i++){
                    if(!enemies[i].is_alive()) continue;
                    int dmg=calc_damage(&enemies[i],subj,true,1.5);
                    apply_damage(&enemies[i],dmg,nullptr,false);
                    cout<<"  对"<<enemies[i].name<<"追加"<<dmg<<"点伤害！"<<endl;
                }
            }
        }else{
            cout<<"[天动万象] 剩余"<<ctx->tian_xian_turns<<"回合"<<endl;
        }
    }
    if(ctx->pi_ya_turns>0){
        ctx->pi_ya_turns--;
        if(ctx->pi_ya_turns==0){
            cout<<"[披星戴月] 强化已结束。"<<endl;
        }else{
            cout<<"[披星戴月] 剩余"<<ctx->pi_ya_turns<<"回合"<<endl;
        }
    }
}

bool BattleSystem::can_summon_yachiyo() {
    if(ctx&&ctx->has_yachiyo()) return false;
    for(auto& b:player->card_buffs) if(b.count("type")&&b["type"]=="summon_yachiyo"&&b["used"]=="0") return true;
    return false;
}

void BattleSystem::summon_yachiyo() {
    for(auto& b:player->card_buffs) if(b.count("type")&&b["type"]=="summon_yachiyo"&&b["used"]=="0") {
        b["used"]="1";
        ctx->yachiyo["hp"]=9999999; ctx->yachiyo["max_hp"]=9999999;
        ctx->yachiyo["atk"]=750; ctx->yachiyo["dfs"]=799;
        ctx->yachiyo_energy=0;
        cout<<"\n月见八千代降临！HP="<<ctx->yachiyo["hp"]<<", ATK="<<ctx->yachiyo["atk"]<<", DFS="<<ctx->yachiyo["dfs"]<<endl;
        cout<<"大招：跨越千年与你相遇(酒寄彩叶) 能量: "<<ctx->yachiyo_energy<<"/"<<ctx->yachiyo_max_energy<<endl;
        return;
    }
}

void BattleSystem::yachiyo_ultimate() {
    if(!ctx||!ctx->has_yachiyo()) return;
    cout<<"\n[月见八千代] 跨越千年与你相遇——酒寄彩叶！！"<<endl;
    int dmg=ctx->yachiyo["atk"]*2;
    for(auto& e:enemies) if(e.is_alive()){
        e.hp=max(0,e.hp-dmg);
        cout<<"  对"<<e.name<<"造成了"<<dmg<<"点伤害！"<<endl;
    }
    ctx->yachiyo_energy=0;
}

void BattleSystem::enemy_phase() {
    // 【挑战7:AI】智能敌方决策: 遍历存活敌人, AI引擎决定每个敌人的行动
    for(size_t i=0;i<enemies.size();i++) {
        auto& e=enemies[i];
        if(!e.is_alive()) continue;
        if(e.immune_turns>0){e.immune_turns--; continue;}
        
        // 【挑战7:AI】构建战斗快照, 调用AI决策引擎
        auto snap = build_ai_snapshot(e);
        AIDecisionEngine::Action action = ai_engine.decide(snap);
        
        // Boss决策: 可选技能/防御/治疗
        if(e.is_boss && !e.skills.empty()) {
            if(action == AIDecisionEngine::SKILL) {
                enemy_use_skill(e);
                cout<<"  ["<<ai_engine.get_action_reason()<<"]"<<endl;
                continue;
            }
            if(action == AIDecisionEngine::HEAL) {
                // 尝试治疗技能
                bool has_heal = false;
                for(auto& sk : e.skills) if(sk=="heal_self") has_heal = true;
                if(has_heal && snap.enemy_hp_pct < 70) {
                    enemy_use_skill(e);
                    cout<<"  ["<<ai_engine.get_action_reason()<<"]"<<endl;
                    continue;
                }
            }
            if(action == AIDecisionEngine::TRUE_DMG) {
                bool has_td = false;
                for(auto& sk : e.skills) if(sk=="true_damage") has_td = true;
                if(has_td) {
                    enemy_use_skill(e);
                    cout<<"  ["<<ai_engine.get_action_reason()<<"]"<<endl;
                    continue;
                }
            }
            if(action == AIDecisionEngine::DEFEND) {
                // Boss防御: 暂时提升防御力模拟
                e.dfs_val = (int)(e.dfs_val * 1.5);
                e.shield += e.dfs_val / 3;
                cout<<"\n"<<e.name<<" 进入防御姿态！防御+50%，获得护盾。"<<endl;
                cout<<"  ["<<ai_engine.get_action_reason()<<"]"<<endl;
                // 防御结束恢复(实际在round_end不恢复,这里简化)
                continue;
            }
            // 默认: 40%概率使用技能(保持一定随机性)
            if(rand_float()<0.4){enemy_use_skill(e);continue;}
        }

        // 【挑战7:AI】月见八千代在场时: AI决策攻击目标
        if(ctx->has_yachiyo()){
            if(action == AIDecisionEngine::TARGET_SUMMON) {
                cout<<"\n"<<e.name<<" [AI决策] 攻击 月见八千代！"<<endl;
                int dmg=max(1,e.atk_val-ctx->yachiyo["dfs"]);
                ctx->yachiyo["hp"]=max(0,ctx->yachiyo["hp"]-dmg);
                ctx->yachiyo_energy=min(ctx->yachiyo_energy+1,ctx->yachiyo_max_energy);
                cout<<e.name<<" 使用"<<e.subject_tag<<"攻击，造成"<<dmg<<"点伤害！"<<endl;
                cout<<"[月见八千代] 受击回复1能量！("<<ctx->yachiyo_energy<<"/"<<ctx->yachiyo_max_energy<<")"<<endl;
                if(ctx->yachiyo["hp"]<=0){cout<<"月见八千代消失了！"<<endl; ctx->yachiyo.clear();}
                continue;
            }
        }

        // 普通攻击逻辑(保持原有学科克制计算)
        string subj=e.subject_tag!="综合"?e.subject_tag:rand_choice(ALL_SUBJECTS);
        int mastery=player->get_subject_mastery(subj);
        int dmg=(max)(1,e.atk_val-player->dfs());
        if(mastery==1) dmg=(int)(dmg*1.5);
        else if(mastery==2) dmg=(int)(dmg*0.5);
        bool refl=false;
        for(auto& b:player->card_buffs) if(b.count("type")&&b["type"]=="reflect_damage"){refl=true;break;}
        if(refl){
            cout<<e.name<<" 使用"<<subj<<"攻击，反弹"<<dmg<<"点伤害！"<<endl;
            e.hp=max(0,e.hp-dmg);
            cout<<"反弹伤害给"<<e.name<<"！"<<endl;
            continue;
        }
        if(player->shield>0){
            if(player->shield>=dmg){player->shield-=dmg; dmg=0;}
            else{dmg-=player->shield; player->shield=0;}
        }
        if(dmg>0){
            player->hp=(max)(0,player->hp-dmg);
            ctx->attack_qi_acc += (has_energy_efficiency()?1.0:0.5);
            if(player->hp<=0){
                for(auto& b:player->card_buffs)
                    if(b.count("type")&&b["type"]=="uniform_survive"&&!player->uniform_survive_used){
                        player->hp=1;player->uniform_survive_used=true;
                        cout<<"[校服守护] HP=1，仅此一次！"<<endl;break;
                    }
                for(auto& b:player->card_buffs)
                    if(b.count("type")&&b["type"]=="survive_charge"&&!player->survive_used){
                        player->hp=1;player->immune_turns=1;player->survive_used=true;
                        player->qi=player->max_qi;
                        cout<<"[卡牌8] HP=1，下回合免疫，充满大招！"<<endl;break;
                    }
                if(player->hp<=0&&player->has_revive){
                    player->hp=player->max_hp;player->has_revive=false;
                    cout<<"[藏品3] 满血复活！"<<endl;
                }
            }
        }
        cout<<e.name<<" 使用"<<subj<<"攻击，造成"<<dmg<<"点伤害！"<<endl;
    }
    if(!player->is_alive()){ ctx->yachiyo.clear(); cout<<"召唤师阵亡，月见八千代消失！"<<endl; }
}

void BattleSystem::enemy_use_skill(Enemy& e) {
    string skill=rand_choice(e.skills);
    if(skill=="aoe_attack"){
        int dmg=(max)(1,e.atk_val-player->dfs());
        cout<<"\n"<<e.name<<" 使用AOE攻击！"<<endl;
        if(ctx->has_yachiyo()) { ctx->yachiyo["hp"]=max(0,ctx->yachiyo["hp"]-dmg); ctx->yachiyo_energy=min(ctx->yachiyo_energy+1,ctx->yachiyo_max_energy); }
        if(player->is_alive()){
            if(player->shield>=dmg) player->shield-=dmg;
            else{dmg-=player->shield;player->shield=0;player->hp=max(0,player->hp-dmg);}
            cout<<"对你造成"<<dmg<<"点伤害！"<<endl;
        }
    }else if(skill=="heal_self"){
        int heal=(int)(e.max_hp*0.2); e.hp=min(e.hp+heal,e.max_hp);
        cout<<"\n"<<e.name<<" 使用恢复，恢复"<<heal<<"点HP！"<<endl;
    }else if(skill=="true_damage"){
        int td=30;
        if(ctx->has_yachiyo()) { ctx->yachiyo["hp"]=max(0,ctx->yachiyo["hp"]-td); ctx->yachiyo_energy=min(ctx->yachiyo_energy+1,ctx->yachiyo_max_energy); }
        player->hp=max(0,player->hp-td);
        cout<<"\n"<<e.name<<" 使用真实伤害攻击，造成"<<td<<"点真实伤害！"<<endl;
    }
}

bool BattleSystem::try_escape() {
    for(auto& e:enemies) if(e.is_boss){cout<<"Boss战不能逃跑！"<<endl;return false;}
    if(rand_float()<0.5){cout<<"逃跑成功！"<<endl;ctx->escaped=true;return true;}
    cout<<"逃跑失败！"<<endl;return false;
}

bool BattleSystem::check_battle_end() {
    bool all_dead=true;
    for(auto& e:enemies) if(e.is_alive()){all_dead=false;break;}
    if(all_dead){battle_victory();return false;}
    if(!player->is_alive()){battle_defeat();return false;}
    return true;
}

// 【考点4.5 战斗系统】战斗胜利: 统计经验+金币奖励, 触发升级, 恢复HP
void BattleSystem::battle_victory() {
    int te=0,tg=0;
    for(auto& e:enemies){te+=e.exp_reward;tg+=e.gold_reward;}  // 【考点6.1 STL::vector】遍历敌人列表
    cout<<"\n"<<string(50,'=')<<endl;
    cout<<"  战斗胜利！"<<endl;
    cout<<"  获得经验: "<<te<<"  获得金币: "<<tg<<endl;
    cout<<string(50,'=')<<endl;
    player->gold+=(int)(tg*player->gold_mult);
    auto[lv,g]=player->gain_exp(te);
    if(lv) cout<<"等级提升！当前 Lv."<<player->level<<endl;
    player->hp=(min)(player->max_hp,player->hp+20);
    cout<<"战斗结束恢复20HP。"<<endl;
}

// 【考点4.5 战斗系统】战斗失败: HP恢复到50%, 不丢失经验
void BattleSystem::battle_defeat() {
    cout<<"\n"<<string(50,'=')<<endl;
    cout<<"  战斗失败！"<<endl<<"  你被击败了..."<<endl;
    cout<<string(50,'=')<<endl;
    player->hp=(int)(player->max_hp*0.5);
    cout<<"HP恢复到最大值的一半: "<<player->hp<<"/"<<player->max_hp<<endl;
}

void BattleSystem::end_of_round() {
    // 第七步: 关卡效果 - N3每回合流失5%最大HP
    if(stage_effects.count("n3_hp_drain")&&stage_effects.at("n3_hp_drain")){
        int drain=(int)(player->max_hp*0.05);
        player->hp=(max)(1,player->hp-drain);
        cout<<"[N3效果] 流失"<<drain<<"点HP！"<<endl;
    }
}

void BattleSystem::battle_loop() {
    while(true){
        ctx->turn++;
        cout<<"\n"<<string(50,'=')<<endl;
        cout<<"  第 "<<ctx->turn<<" 回合"<<endl;
        cout<<string(50,'=')<<endl;
        if(is_player_turn){
            player_phase();
            if(ctx->escaped||!check_battle_end()){delete ctx;ctx=nullptr;return;}
            is_player_turn=false;
        }else{
            enemy_phase();
            if(!check_battle_end()){delete ctx;ctx=nullptr;return;}
            is_player_turn=true;
            end_of_round();
        }
    }
}

// ============================================
// player_phase: 玩家回合主流程(按优先级依次执行)
// 步骤:
//   1. 免疫状态检查(卡牌8无敌回合)
//   2. 回合开始获得气(1或2点)
//   3. buff过期检查(天动万象/披星戴月)
//   4. 月见八千代获得能量
//   5. 希望校服回血(回合开始时)
//   6. 显示状态面板(HP/气/护盾/敌人)
//   7. 显示行动菜单并等待选择
// ============================================
void BattleSystem::player_phase() {
    // 步骤1: 免疫状态 - 直接跳过行动
    if(player->immune_turns>0){
        cout<<"你处于免疫状态(剩余"<<player->immune_turns<<"回合)！"<<endl;
        player->immune_turns--; return;
    }
    // 攻击获得2气(含效率翻倍则4), 受攻击累积0.5气
    int qi_gain=has_energy_efficiency()?4:2;
    player->gain_qi(qi_gain);
    if(player->qi_overflow>0){player->qi+=player->qi_overflow;player->qi_overflow=0;player->qi=min(player->qi,player->max_qi);}
    // 处理受击累积的气
    if(ctx->attack_qi_acc>=1.0){int g=(int)ctx->attack_qi_acc;player->gain_qi(g);ctx->attack_qi_acc-=g;}

    process_buff_expiry();

    if(ctx->has_yachiyo()){
        ctx->yachiyo_energy=min(ctx->yachiyo_energy+1,ctx->yachiyo_max_energy);
        cout<<"[月见八千代] 获得1能量！("<<ctx->yachiyo_energy<<"/"<<ctx->yachiyo_max_energy<<")"<<endl;
    }

    for (auto& b : player->card_buffs)
        if (b.count("type") && b["type"]=="hope_uniform") {
            int pct=stoi(b["value"]);
            int heal=(int)(player->max_hp*pct/100.0);
            int before=player->hp;
            player->hp=min(player->max_hp,player->hp+heal);
            if(player->hp>before) cout<<"[希望校服] 恢复了"<<(player->hp-before)<<"点HP！"<<endl;
            break;
        }

    string pi_ya_tag = "";
    if(ctx->has_pi_ya_turns()) pi_ya_tag=" [披星戴月]";
    string qi_tag="";
    if(has_energy_efficiency()) qi_tag=" x2";
    cout<<"\n你的状态: HP "<<player->hp<<"/"<<player->max_hp<<" | 气 "<<player->qi<<"/"<<player->max_qi<<" | 护盾 "<<player->shield<<pi_ya_tag<<endl;
    if(ctx->has_yachiyo()){
        cout<<"月见八千代: HP "<<ctx->yachiyo["hp"]<<"/"<<ctx->yachiyo["max_hp"]<<" | 能量 "<<ctx->yachiyo_energy<<"/"<<ctx->yachiyo_max_energy<<endl;
    }
    cout<<"敌人状态:"<<endl;
    for(size_t i=0;i<enemies.size();i++) if(enemies[i].is_alive()){
        string si=enemies[i].shield>0?" 护盾"+to_string(enemies[i].shield):"";
        cout<<"  ["<<(i+1)<<"] "<<enemies[i].name<<" - HP "<<enemies[i].hp<<"/"<<enemies[i].max_hp<<si<<endl;
    }
    cout<<"\n"<<string(40,'-')<<endl;
    // 【挑战2:多线程】30秒回合计时器
    start_battle_timer();
    int timer_seconds = 30;
    while(true){
    cout<<"  行动选择:"<<endl;
    cout<<"  1. 普通攻击\n  2. 使用药品\n  3. 查看卡牌效果\n  4. 角色大招\n  5. 防御\n";
    int opt=6;
    if(can_summon_yachiyo()){ cout<<"  "<<opt<<". 召唤月见八千代\n"; opt++; }
    else if(ctx->has_yachiyo()&&ctx->can_yachiyo_ultimate()){ cout<<"  "<<opt<<". 月见八千代·大招\n"; opt++; }
    cout<<"  0. 逃跑\n"<<string(40,'-')<<endl;
    // 【挑战2:多线程】显示回合剩余时间
    if(battle_timer_enabled) {
        auto now = chrono::steady_clock::now();
        int elapsed = (int)chrono::duration_cast<chrono::seconds>(now - battle_timer_start).count();
        int remaining = max(0, timer_seconds - elapsed);
        if(remaining <= 10) cout<<"  ⚠ 剩余时间: "<<remaining<<"秒！请尽快行动！"<<endl;
    }
    string ch; cout<<"请选择行动: "; getline(cin,ch);
    
    // 【挑战2:多线程】检查回合计时器是否超时(30秒限制)
    if(battle_timer_enabled && is_battle_timer_expired(timer_seconds)) {
        cout<<"\n  ⏰ 回合时间耗尽！自动进入防御姿态！"<<endl;
        player_defend(); break;
    }
    
    if(ch=="1"){ player_normal_attack(); break; }
    else if(ch=="2"){ player_use_item(); break; }
    else if(ch=="3"){ player_use_card(); break; }
    else if(ch=="4"){
        if(player_use_ultimate()) break;
    }
    else if(ch=="5"){ player_defend(); break; }
    else if(ch=="6"&&can_summon_yachiyo()){ summon_yachiyo(); break; }
    else if(ch=="6"&&ctx->has_yachiyo()&&ctx->can_yachiyo_ultimate()){ yachiyo_ultimate(); break; }
    else if(ch=="0"){if(try_escape())return;}
    else cout<<"无效选择！"<<endl;
    }
}

// ============================================
// player_defend: 防御姿态
// 无卡牌6: 获得DFS值的护盾
// 有卡牌6: 获得9999护盾(免疫本次伤害)
// 同时回复1或2点气
// ============================================
void BattleSystem::player_defend() {
    cout<<"\n你进入防御姿态！"<<endl;
    bool has_defend_immune=player->has_card_buff_type("defend_immune");
    if(has_defend_immune){
        player->shield+=9999;
        cout<<"[卡牌6] 本次受到伤害免疫！"<<endl;
    }else{
        player->shield+=player->dfs();
        cout<<"护盾+"<<player->dfs()<<"点！"<<endl;
    }
    int qi_gain=has_energy_efficiency()?2:1;
    player->gain_qi(qi_gain);
    cout<<"防御回复"<<qi_gain<<"点气！当前气: "<<player->qi<<"/"<<player->max_qi<<endl;
}

void BattleSystem::start_battle(vector<Enemy> enems, map<string,bool> effects) {
    ctx=new BattleContext(); enemies=enems; stage_effects=effects;
    player->reset_battle_state();
    for(auto& e:enemies){e.shield=0;e.card_buffs.clear();e.survive_used=false;e.immune_turns=0;}
    is_player_turn=true;
    start_battle_timer();  // 【挑战2:多线程】初始化回合计时
    battle_loop();
}

// ============================================
// 第六部分: 背包系统 (InventorySystem)
// 提供物品的查看/使用/删除功能
// 支持运行时类型识别(dynamic_cast)区分装备和消耗品
// ============================================
// 【考点4.2 背包管理】查看背包: 遍历inventory map展示所有物品
// ============================================
void InventorySystem::view_items(){
    if(player->inventory.empty()){cout<<"\n背包是空的。"<<endl;return;}
    cout<<"\n"<<string(50,'=')<<endl<<"  [背包物品]"<<endl<<string(50,'-')<<endl;
    item_index.clear(); int i=1;
    for(auto&[id,qty]:player->inventory){
        auto it=ITEMS.find(id);
        if(it!=ITEMS.end()){
            string ic="";
            if(it->second->item_type=="book") ic="[课本]";
            else if(it->second->item_type=="potion") ic="[药品]";
            else if(it->second->item_type=="equipment_clothes") ic="[衣服]";
            else if(it->second->item_type=="equipment_tool") ic="[文具]";
            else ic="[其他]";
            cout<<"  ["<<i<<"] "<<ic<<" "<<it->second->name<<" x"<<qty<<endl;
            item_index[i]={id,it->second}; i++;
        }
    }
    cout<<string(50,'=')<<endl;
}

// 【考点4.2 背包管理】使用物品: dynamic_cast运行时类型识别区分装备/药品
// 【考点1.3 多态】通过dynamic_cast在运行时判断Item具体子类, 执行不同逻辑
void InventorySystem::use_item(){
    view_items(); if(player->inventory.empty()) return;
    string s; cout<<"\n选择要使用的物品编号(0=取消): "; getline(cin,s);
    try{int ch=stoi(s); if(ch==0)return; auto it=item_index.find(ch);
        if(it==item_index.end()){cout<<"无效选择！"<<endl;return;}
        auto[id,item]=it->second;
        if(Equipment* eq=dynamic_cast<Equipment*>(item)){
            cout<<"\n装备 "<<item->name<<"？(y/n): "; string conf; getline(cin,conf);
            if(conf=="y"||conf=="Y"){
                string slot=eq->equip_type;
                auto oi=player->equipment.find(slot);
                if(oi!=player->equipment.end()&&oi->second) player->add_item(oi->second->item_id);
                player->equip(eq); player->remove_item(id);
                cout<<"已装备 "<<item->name<<"！"<<endl;
            }
        }else{
            auto[succ,msg]=item->use(*player);
            if(succ) player->remove_item(id);
            cout<<"\n"<<msg<<endl;
        }
    }catch(...){cout<<"请输入有效数字！"<<endl;}
}

// 【考点4.2 背包管理】删除物品: 确认后从inventory中移除
void InventorySystem::delete_item(){
    view_items(); if(player->inventory.empty()) return;
    string s; cout<<"\n选择要删除的物品编号(0=取消): "; getline(cin,s);
    try{int ch=stoi(s); if(ch==0)return; auto it=item_index.find(ch);
        if(it==item_index.end()){cout<<"无效选择！"<<endl;return;}
        auto[id,item]=it->second;
        cout<<"确认删除 "<<item->name<<"？(y/n): "; string conf; getline(cin,conf);
        if(conf=="y"||conf=="Y"){player->remove_item(id); cout<<"已删除 "<<item->name<<"！"<<endl;}
    }catch(...){cout<<"请输入有效数字！"<<endl;}
}

void InventorySystem::run(){
    while(true){
        cout<<"\n"<<string(40,'=')<<endl<<"  背包系统"<<endl;
        cout<<"  1. 查看物品\n  2. 使用物品\n  3. 删除物品\n  0. 返回"<<endl;
        cout<<string(40,'=')<<endl;
        string ch; cout<<"请选择: "; getline(cin,ch);
        if(ch=="1") view_items();
        else if(ch=="2") use_item();
        else if(ch=="3") delete_item();
        else if(ch=="0") break;
        else cout<<"无效选择！"<<endl;
    }
}

// ============================================
// 第七部分: 商店系统 (ShopSystem)
// 小卖铺: 4层货架(绿/蓝/紫/金品质装备)
// 医院药房: 内科(回血药品) + 强化科(属性药品)
// 出售: 物品以50%原价回收
// ============================================
// ============================================
string ShopSystem::get_rarity_color(const string& rarity) {
    if (rarity=="绿色") return "\033[32m";
    if (rarity=="蓝色") return "\033[34m";
    if (rarity=="紫色") return "\033[35m";
    if (rarity=="金色") return "\033[33m";
    return "\033[0m";
}

void ShopSystem::view_items(int level_filter){
    cout<<"\n"<<string(60,'=')<<endl;
    cout<<"  校园商店 (你的金币: "<<player->gold<<")"<<endl;
    cout<<string(60,'-')<<endl;

    shop_idx.clear();
    int idx=1;
    vector<pair<string,const vector<string>*>> shelves={
        {"第一货架 - 绿色品质(初级装备)", &SHOP_LEVEL1},
        {"第二货架 - 蓝色品质(中级装备)", &SHOP_LEVEL2},
        {"第三货架 - 紫色品质(高级装备)", &SHOP_LEVEL3},
        {"第四货架 - 金色品质(稀有装备)", &SHOP_LEVEL4}
    };

    for (auto& [name, items] : shelves) {
        cout<<"\n  --- "<<name<<" ---"<<endl;
        for (auto& item_id : *items) {
            auto it=ITEMS.find(item_id);
            if (it!=ITEMS.end()) {
                Equipment* eq=dynamic_cast<Equipment*>(it->second);
                string tn="其他", rc="";
                if (eq) {
                    tn="装备";
                    if(eq->rarity=="绿色"){rc="[绿]";} else if(eq->rarity=="蓝色"){rc="[蓝]";}
                    else if(eq->rarity=="紫色"){rc="[紫]";} else if(eq->rarity=="金色"){rc="[金]";}
                } else if(it->second->item_type=="potion") tn="药品";
                printf("  [%-3d] %-20s %-4s %-6dG %s\n",idx,(rc+it->second->name).c_str(),tn.c_str(),it->second->price,it->second->desc.c_str());
                shop_idx[idx]={item_id,it->second}; idx++;
            }
        }
    }
    cout<<string(60,'-')<<endl;
    cout<<"  装备品质: [绿]初级 [蓝]中级 [紫]高级 [金]稀有 | 出售回收50%价格"<<endl;
    cout<<string(60,'=')<<endl;
}

void ShopSystem::buy_item(){
    view_items();
    string s; cout<<"\n选择要购买的商品编号(0=取消): "; getline(cin,s);
    try{int ch=stoi(s); if(ch==0)return; auto it=shop_idx.find(ch);
        if(it==shop_idx.end()){cout<<"无效选择！"<<endl;return;}
        auto[id,item]=it->second;
        if(player->gold<item->price){cout<<"金币不足！"<<endl;return;}
        player->gold-=item->price; player->add_item(id);
        cout<<"购买了 "<<item->name<<"！"<<endl;
    }catch(...){cout<<"请输入有效数字！"<<endl;}
}

void ShopSystem::sell_item(){
    if(player->inventory.empty()){cout<<"背包是空的。"<<endl;return;}
    cout<<"\n你的背包："<<endl;
    map<int,pair<string,Item*>> sl; int i=1;
    for(auto&[id,qty]:player->inventory){
        auto it=ITEMS.find(id);
        if(it!=ITEMS.end()){
            int sp=it->second->price/2;
            cout<<"  ["<<i<<"] "<<it->second->name<<" x"<<qty<<" (回收价: "<<sp<<"G)"<<endl;
            sl[i]={id,it->second}; i++;
        }
    }
    string s; cout<<"\n选择要出售的物品编号(0=取消): "; getline(cin,s);
    try{int ch=stoi(s); if(ch==0)return; auto it=sl.find(ch);
        if(it==sl.end()){cout<<"无效选择！"<<endl;return;}
        auto[id,item]=it->second; int sp=item->price/2;
        player->remove_item(id); player->gold+=sp;
        cout<<"出售了 "<<item->name<<"，获得"<<sp<<"G！"<<endl;
    }catch(...){cout<<"请输入有效数字！"<<endl;}
}

// 【考点4.3 商店系统】药房: 购买药品,分为内科(回复)和强化科(属性提升)
void ShopSystem::pharmacy(){
    cout<<"\n"<<string(60,'=')<<endl;
    cout<<"  校园医院药房 (你的金币: "<<player->gold<<" | HP: "<<player->hp<<"/"<<player->max_hp<<")"<<endl;
    cout<<string(60,'-')<<endl;
    cout<<"  提示: 药品可在战斗中使用，也可在背包中随时使用。"<<endl;
    cout<<string(60,'-')<<endl;

    vector<pair<string,Item*>> meds;
    vector<pair<string,Item*>> dept_restore;  // 内科 - 回复
    vector<pair<string,Item*>> dept_enhance;  // 强化科 - 属性

    for (auto& item_id : PHARMACY_ITEMS) {
        auto it=ITEMS.find(item_id);
        if (it!=ITEMS.end()) {
            Potion* pot=dynamic_cast<Potion*>(it->second);
            if (pot && pot->hp_restore>0 && pot->atk_boost==0 && pot->dfs_boost==0)
                dept_restore.push_back({item_id,it->second});
            else
                dept_enhance.push_back({item_id,it->second});
        }
    }

    int idx=1;
    cout<<"\n  --- [内科] 回复药品 ---"<<endl;
    for (auto& [id,item] : dept_restore) {
        cout<<"  ["<<idx<<"] "<<item->name<<" - "<<item->price<<"G - "<<item->desc<<endl;
        meds.push_back({id,item}); idx++;
    }

    cout<<"\n  --- [强化科] 属性药品 ---"<<endl;
    for (auto& [id,item] : dept_enhance) {
        cout<<"  ["<<idx<<"] "<<item->name<<" - "<<item->price<<"G - "<<item->desc<<endl;
        meds.push_back({id,item}); idx++;
    }

    cout<<string(60,'=')<<endl;
    string s; cout<<"\n选择要购买的药品编号(0=取消): "; getline(cin,s);
    try{int ch=stoi(s); if(ch==0)return;
        if(ch>=1&&(size_t)(ch-1)<meds.size()){
            auto[id,item]=meds[ch-1];
            if(player->gold<item->price){cout<<"金币不足！"<<endl;return;}
            player->gold-=item->price; player->add_item(id);
            cout<<"购买了 "<<item->name<<"！"<<endl;
        }else{cout<<"无效选择！"<<endl;}
    }catch(...){cout<<"请输入有效数字！"<<endl;}
}

void ShopSystem::run(){
    while(true){
        cout<<"\n"<<string(40,'=')<<endl<<"  校园商店"<<endl;
        cout<<"  1. 查看商品列表\n  2. 购买商品\n  3. 出售商品\n  0. 返回"<<endl;
        cout<<string(40,'=')<<endl;
        string ch; cout<<"请选择: "; getline(cin,ch);
        if(ch=="1") view_items();
        else if(ch=="2") buy_item();
        else if(ch=="3") sell_item();
        else if(ch=="0") break;
        else cout<<"无效选择！"<<endl;
    }
}

// ============================================
// 第八部分: 任务系统 (QuestSystem)
// 6个日常任务, 接受后执行即可完成
// 奖励: 经验+金币+随机物品
// ============================================
// ============================================
void QuestSystem::view(){
    cout<<"\n"<<string(50,'=')<<endl<<"  任务列表"<<endl<<string(50,'-')<<endl;
    for(size_t i=0;i<QUESTS.size();i++){
        string st=QUESTS[i].completed?"✓":"○";
        cout<<"  ["<<(i+1)<<"] "<<st<<" "<<QUESTS[i].name<<endl;
        cout<<"      "<<QUESTS[i].desc<<endl;
        cout<<"      奖励: "<<QUESTS[i].get_reward_desc()<<endl;
    }
    cout<<string(50,'=')<<endl;
}

// 【考点4.4 任务系统】接受并执行任务
void QuestSystem::accept(){
    view();
    string s; cout<<"\n选择要执行的任务编号(0=取消): "; getline(cin,s);
    try{int ch=stoi(s); if(ch==0)return;
        if(ch>=1&&(size_t)(ch-1)<QUESTS.size()){
            auto& q=QUESTS[ch-1]; q.reset();
            cout<<"\n接取了任务: "<<q.name<<endl;
            do_quest(q);
    }}catch(...){cout<<"请输入有效数字！"<<endl;}
}

// 【考点4.4 任务系统】执行: 进度+1, 达到目标后自动完成
void QuestSystem::do_quest(Quest& q){
    q.progress++;
    map<string,string> msgs={
        {"sweep","你挥舞着扫帚，把教室打扫得一尘不染。"},
        {"help_teacher","你帮老师批改了一摞作业，老师对你刮目相看。"},
        {"homework","你把作业工工整整地完成了。"},
        {"quiz","你从容应对随堂测验，发挥出色。"},
        {"running","你在操场上挥洒汗水，完成了晨跑。"},
        {"study","你在图书馆翻阅了大量参考资料。"}
    };
    cout<<"\n"<<(msgs.count(q.quest_type)?msgs[q.quest_type]:"你完成了任务。")<<endl;
    if(q.is_completed()) complete_quest(q);
}

// 【考点4.4 任务系统】完成任务: 发放经验/金币/物品奖励
// 【考点6.1 STL::vector】completed_quests记录已完成任务
void QuestSystem::complete_quest(Quest& q){
    cout<<"\n任务完成: "<<q.name<<"！"<<endl;
    cout<<"获得奖励: "<<q.get_reward_desc()<<endl;
    player->gold+=q.gold_reward; player->gain_exp(q.exp_reward);
    if(!q.item_reward.empty()) player->add_item(q.item_reward);
    q.completed=true; player->completed_quests.push_back(q.quest_id);
}

void QuestSystem::run(){
    while(true){
        cout<<"\n"<<string(40,'=')<<endl<<"  任务系统"<<endl;
        cout<<"  1. 查看任务\n  2. 执行任务\n  0. 返回"<<endl;
        cout<<string(40,'=')<<endl;
        string ch; cout<<"请选择: "; getline(cin,ch);
        if(ch=="1") view();
        else if(ch=="2") accept();
        else if(ch=="0") break;
        else cout<<"无效选择！"<<endl;
    }
}

// ============================================
// 第九部分: 成长系统 (GrowthSystem)
// 展示当前等级/经验/经验倍率
// 经验倍率受藏品2(效率)影响
// ============================================
// ============================================
void GrowthSystem::view_exp(){
    cout<<"\n当前经验: "<<player->exp<<"/"<<player->exp_to_next<<endl;
    cout<<"等级: Lv."<<player->level<<endl;
    cout<<"经验倍率: x"<<player->exp_mult<<endl;
}

void GrowthSystem::run(){
    while(true){
        cout<<"\n"<<string(40,'=')<<endl<<"  成长系统"<<endl;
        cout<<"  1. 查看经验值\n  0. 返回"<<endl;
        cout<<string(40,'=')<<endl;
        string ch; cout<<"请选择: "; getline(cin,ch);
        if(ch=="1") view_exp();
        else if(ch=="0") break;
        else cout<<"无效选择！"<<endl;
    }
}

// ============================================
// Challenge 6: STL高级应用 - 加权抽牌函数
// 使用priority_queue对可抽卡牌按power_level排序
// 使用discrete_distribution实现加权随机抽取
// power_level越高的卡牌被抽到的概率越大
// ============================================
Card* draw_weighted_card(Player& player) {
    // 第一步: 用priority_queue按power_level排序
    priority_queue<CardPriority> pq;
    for(auto* c : CARDS) {
        if(!player.has_card_buff_type(c->effect_type)) {
            int power = CARD_POWER.count(c->effect_type) ? CARD_POWER[c->effect_type] : 5;
            pq.push(CardPriority(c, power));
        }
    }
    if(pq.empty()) return nullptr;
    
    // 第二步: 从优先级堆中提取card和对应的weight
    vector<Card*> cards;
    vector<double> weights;
    while(!pq.empty()) {
        auto cp = pq.top(); pq.pop();
        cards.push_back(cp.card);
        weights.push_back((double)cp.power_level);
    }
    
    // 第三步: 使用discrete_distribution加权随机选一张
    return weighted_choice(cards, weights);
}

// ============================================
// 第十部分: 关卡系统 (StageSystem)
// 协调关卡挑战的完整流程:
//   小怪×3 → 补给点 → 小怪×3 → 补给点 → 小怪×3 → 补给点 → Boss → 结算
// 补给点随机获得卡牌(50%)或物品(50%)
// 通关随机获得藏品(30%)+卡牌
// 通关后可扫荡
// ============================================
// ============================================
void StageSystem::view_stages() {
    cout<<"\n"<<string(60,'=')<<endl<<"  关卡选择"<<endl<<string(60,'-')<<endl;
    vector<tuple<string,string,string>> stages={
        {"N0","初入校园","初始难度，适合新手适应战斗系统"},
        {"N1","周末修养","敌方学识和抗压+10%"},
        {"N2","月末检测","敌方+10%属性，我方-10%属性"},
        {"N3","中流击楫","N2基础上+每回合流失5%HP"},
        {"N4","期末定音","N3基础上+禁止使用药品"},
        {"N5","一战登天","N4基础上+Boss附带真实伤害和吸血"},
    };
    for(auto&[sid,name,desc]:stages){
        string done=stage_progress.count(sid)&&stage_progress[sid]?"✓":"○";
        cout<<"  ["<<sid<<"] "<<done<<" "<<name<<endl;
        cout<<"      "<<desc<<endl<<endl;
    }
    cout<<string(60,'-')<<endl;
    cout<<"  注意事项:"<<endl;
    cout<<"  - 通关后可开启扫荡功能"<<endl;
    cout<<"  - 每关内有中间补给点(可获取卡牌或道具)"<<endl;
    cout<<"  - 血量在各小关之间继承"<<endl;
    cout<<string(60,'=')<<endl;
}

void StageSystem::view_cards_and_treasures() {
    cout<<"\n"<<string(60,'=')<<endl<<"  卡牌介绍"<<endl<<string(60,'-')<<endl;
    for(auto* c:CARDS){
        cout<<"  ["<<c->card_id<<"] "<<c->name<<endl;
        cout<<"      效果: "<<c->desc<<endl;
        if(player->has_card_buff_type(c->effect_type)) cout<<"      (已获得)"<<endl;
        cout<<endl;
    }
    cout<<string(60,'=')<<endl;
    cout<<"\n"<<string(60,'=')<<endl<<"  秘宝(藏品)介绍"<<endl<<string(60,'-')<<endl;
    for(auto* t:TREASURES){
        cout<<"  ["<<t->treasure_id<<"] "<<t->name<<endl;
        cout<<"      效果: "<<(t->effect_desc.empty()?t->desc:t->effect_desc)<<endl;
        if(has_treasure(t->name)) cout<<"      (已获得)"<<endl;
        cout<<endl;
    }
    cout<<string(60,'=')<<endl;
}

bool StageSystem::has_treasure(const string& name) {
    return find(player->treasures.begin(),player->treasures.end(),name)!=player->treasures.end();
}

bool StageSystem::has_card_type(const string& card_type) {
    return player->has_card_buff_type(card_type);
}

bool StageSystem::fight_wave(map<string,bool>& cfg, int level, int wave) {
    vector<string> subj_keys;
    for(auto&[k,v]:SUBJECT_ENEMIES) subj_keys.push_back(k);
    vector<string> picked;
    vector<int> indices(subj_keys.size());
    for(size_t i=0;i<subj_keys.size();i++) indices[i]=(int)i;
    shuffle(indices.begin(),indices.end(),rng);
    int count=(min)(3,(int)subj_keys.size());
    vector<Enemy> enemies;
    for(int i=0;i<count;i++){
        string key=subj_keys[indices[i]];
        Enemy base=ENEMIES_MAP[SUBJECT_ENEMIES[key]];
        enemies.push_back(create_stage_enemy(base,level));
    }
    battle_sys.start_battle(enemies,cfg);
    player->hp=(min)(player->max_hp,player->hp+15);
    cout<<"战后恢复15HP。当前HP: "<<player->hp<<"/"<<player->max_hp<<endl;
    return player->is_alive();
}

bool StageSystem::fight_boss(map<string,bool>& cfg, int level) {
    string boss_key;
    if(level<=2) boss_key="boss_midterm";
    else if(level<=4) boss_key="boss_final";
    else boss_key="boss_entrance";
    Enemy boss=create_stage_enemy(ENEMIES_MAP[boss_key],level);
    auto effects=cfg;
    if(effects.count("n5_enemy_bonus")&&effects.at("n5_enemy_bonus")){
        cout<<"[N5效果] Boss召唤了一个相同属性的小怪！"<<endl;
        vector<string> keys;
        for(auto&[k,v]:SUBJECT_ENEMIES) keys.push_back(k);
        string key=rand_choice(keys);
        Enemy minion=create_stage_enemy(ENEMIES_MAP[SUBJECT_ENEMIES[key]],level);
        battle_sys.start_battle({boss,minion},effects);
    }else{
        battle_sys.start_battle({boss},effects);
    }
    return player->is_alive();
}

void StageSystem::rest_point(int wave) {
    cout<<"\n找到补给点！"<<endl;
    if(rand_float()<0.5){
        vector<Card*> avail_cards;
        for (auto* c : CARDS)
            if (!player->has_card_buff_type(c->effect_type))
                avail_cards.push_back(c);
        if (!avail_cards.empty()) {
            Card* card=rand_choice(avail_cards);
            cout<<"获得卡牌: "<<card->name<<endl;
            auto[succ,msg]=card->apply(*player);
            if(!msg.empty()) cout<<"  效果: "<<msg<<endl;
        } else {
            cout<<"所有卡牌效果已获得！获得金币补偿。"<<endl;
            player->gold+=50;
        }
    }else{
        string item_id=rand_choice(SHOP_ITEMS);
        if(ITEMS.count(item_id)){
            player->add_item(item_id);
            cout<<"获得物品: "<<ITEMS[item_id]->name<<endl;
        }
    }
}

void StageSystem::start_stage(const string& stage_id) {
    map<string,map<string,bool>> cfg_map={
        {"N0",{}},
        {"N1",{}},
        {"N2",{{"player_debuff",true}}},
        {"N3",{{"player_debuff",true},{"n3_hp_drain",true}}},
        {"N4",{{"player_debuff",true},{"n3_hp_drain",true},{"no_potion",true}}},
        {"N5",{{"player_debuff",true},{"n3_hp_drain",true},{"no_potion",true},{"n5_enemy_bonus",true}}},
    };
    if(!cfg_map.count(stage_id)) cfg_map[stage_id]={};
    auto cfg=cfg_map[stage_id];
    int level_num=stage_id[1]-'0';

    if(cfg.count("player_debuff")&&cfg.at("player_debuff")){
        player->atk_mult*=0.9; player->dfs_mult*=0.9;
    }

    cout<<"\n"<<string(50,'=')<<endl;
    cout<<"  进入关卡: "<<stage_id<<endl;
    cout<<string(50,'=')<<endl;

    bool stage_cleared=false;  // 【新增】追踪通关状态

    vector<string> sn={"日常作业","周中测","周测","月考","期中考试","期末考试"};
    int lv=1;
    for(int bi=0;bi<6;bi++){
        cout<<"\n--- 第"<<lv++<<"小关: "<<sn[bi]<<" ---"<<endl;
        if(!player->is_alive()) goto stage_end;
        if(bi==5){ if(!fight_boss(cfg,level_num)) goto stage_end; }
        else{ if(!fight_wave(cfg,level_num,bi+1)) goto stage_end; }
        if(!player->is_alive()) goto stage_end;
        pause_msg();
        if(player->is_alive()&&bi<5){
            cout<<"\n--- 补给: 抽取卡牌 ---"<<endl;
            // 【挑战6:STL高级应用】使用priority_queue+加权随机替代简单rand_choice
            Card* card=draw_weighted_card(*player);
            if(card){ cout<<"获得卡牌: "<<card->name<<" - "<<card->desc<<endl; auto[succ,msg]=card->apply(*player); if(!msg.empty()) cout<<msg<<endl; }
            else{ cout<<"卡牌已齐，金币+50G。"<<endl; player->gold+=50; }
        }
        if(bi<5){
            cout<<"\n--- 第"<<lv++<<"小关: 奖励关 · 藏品 ---"<<endl;
            vector<Treasure*> at;
            for(auto* t:TREASURES) if(!has_treasure(t->name)) at.push_back(t);
            if(!at.empty()){ Treasure* t=rand_choice(at); player->treasures.push_back(t->name); cout<<"获得秘宝: "<<t->name<<"！"<<endl<<t->apply(*player)<<endl; }
            else{ cout<<"藏品已齐，金币+100G。"<<endl; player->gold+=100; }
            pause_msg();
        }
    }

    cout<<"\n"<<string(50,'=')<<endl;
    cout<<"  恭喜通关 "<<stage_id<<"！"<<endl;
    cout<<string(50,'=')<<endl;
    stage_progress[stage_id]=true;
    stage_cleared=true;  // 【新增】标记通关成功

stage_end:
    player->record_stage_clear(stage_id, stage_cleared, 0);  // 【新增】记录关卡历史
    if(cfg.count("player_debuff")&&cfg.at("player_debuff")){
        player->atk_mult/=0.9; player->dfs_mult/=0.9;
    }
}

void StageSystem::sweep_stage(const string& stage_id) {
    if(!stage_progress.count(stage_id)||!stage_progress[stage_id]){
        cout<<"尚未通关"<<stage_id<<"，无法扫荡！"<<endl; return;
    }
    cout<<"\n开始扫荡 "<<stage_id<<"..."<<endl;
    int eg=50+(stage_id[1]-'0')*30;
    int gg=30+(stage_id[1]-'0')*20;
    player->gold+=gg;
    auto[lv,gn]=player->gain_exp(eg);
    cout<<"获得经验: "<<eg<<", 金币: "<<gg<<endl;
    if(lv) cout<<"等级提升！当前 Lv."<<player->level<<endl;
}

void StageSystem::run(){
    while(true){
        cout<<"\n"<<string(40,'=')<<endl<<"  关卡系统"<<endl;
        cout<<"  1. 查看关卡与介绍"<<endl;
        cout<<"  2. 选择关卡挑战"<<endl;
        cout<<"  3. 扫荡(已通关卡)"<<endl;
        cout<<"  4. 卡牌与藏品介绍"<<endl;
        cout<<"  0. 返回"<<endl;
        cout<<string(40,'=')<<endl;
        string ch; cout<<"请选择: "; getline(cin,ch);
        if(ch=="1") view_stages();
        else if(ch=="2"){
            view_stages();
            string stage; cout<<"\n输入关卡编号(如 N0, N1...): "; getline(cin,stage);
            for(auto& c:stage) c=toupper(c);
            if(stage=="N0"||stage=="N1"||stage=="N2"||stage=="N3"||stage=="N4"||stage=="N5"){
                if(player->hp<=0) cout<<"你的HP不足，先去医院治疗吧！"<<endl;
                else start_stage(stage);
            }else cout<<"无效的关卡编号！"<<endl;
        }
        else if(ch=="3"){
            view_stages();
            string stage; cout<<"\n输入要扫荡的关卡编号: "; getline(cin,stage);
            for(auto& c:stage) c=toupper(c);
            sweep_stage(stage);
        }
        else if(ch=="4") view_cards_and_treasures();
        else if(ch=="0") break;
        else cout<<"无效选择！"<<endl;
    }
}

// ============================================
// 【新增】成长数据可视化面板
// 功能: 生成包含Chart.js图表的HTML文件, 自动在浏览器中打开
// 图表: ①成长曲线 ②学科雷达图 ③关卡通关历史 ④物品使用频率
// 依赖: Chart.js CDN (运行时加载, 需联网)
// ============================================
string generate_dashboard_html(const Player& player) {
    ostringstream js_data, html;
    
    // ======== 构建Chart.js数据 ========
    // 1. 成长曲线数据
    js_data << "const growthLabels=[";
    for(size_t i=0;i<player.growth_snapshots.size();i++){
        if(i>0) js_data<<",";
        js_data<<player.growth_snapshots[i].level;
    }
    js_data << "];const growthHP=[";
    for(size_t i=0;i<player.growth_snapshots.size();i++){
        if(i>0) js_data<<",";
        js_data<<player.growth_snapshots[i].max_hp;
    }
    js_data << "];const growthATK=[";
    for(size_t i=0;i<player.growth_snapshots.size();i++){
        if(i>0) js_data<<",";
        js_data<<player.growth_snapshots[i].atk;
    }
    js_data << "];const growthDFS=[";
    for(size_t i=0;i<player.growth_snapshots.size();i++){
        if(i>0) js_data<<",";
        js_data<<player.growth_snapshots[i].dfs;
    }
    js_data << "];";
    
    // 2. 学科雷达图 - 精通=90, 了解=50, 未掌握=20
    js_data << "const radarLabels=[";
    bool first=true;
    for(auto& s:ALL_SUBJECTS){
        if(!first) js_data<<",";
        first=false;
        js_data<<"'"<<s<<"'";
    }
    js_data << ",'HP','ATK','DFS'];const radarData=[";
    first=true;
    for(auto& s:ALL_SUBJECTS){
        if(!first) js_data<<",";
        first=false;
        auto it=player.subjects.find(s);
        if(it!=player.subjects.end()){
            js_data<<(it->second=="精通"?90:(it->second=="了解"?50:20));
        }else js_data<<20;
    }
    // HP/ATK/DFS归一化到0-100
    int hp_norm=min(100,(int)(player.max_hp*100.0/800));
    int atk_norm=min(100,(int)(player.atk()*100.0/300));
    int dfs_norm=min(100,(int)(player.dfs()*100.0/200));
    js_data<<","<<hp_norm<<","<<atk_norm<<","<<dfs_norm<<"];";
    
    // 3. 关卡通关历史
    js_data << "const stageLabels=[";
    first=true;
    for(auto& sr:player.stage_records){
        if(!first) js_data<<",";
        first=false;
        js_data<<"'"<<sr.stage_name.substr(0,4)<<"'";
    }
    js_data << "];const stageData=[";
    first=true;
    for(auto& sr:player.stage_records){
        if(!first) js_data<<",";
        first=false;
        js_data<<(sr.cleared?5:1);
    }
    js_data << "];const stageBg=[";
    first=true;
    for(auto& sr:player.stage_records){
        if(!first) js_data<<",";
        first=false;
        js_data<<"'"<<(sr.cleared?"rgba(0,200,83,0.7)":"rgba(255,82,82,0.7)")<<"'";
    }
    js_data << "];";
    
    // 4. 物品使用频率
    js_data << "const itemLabels=[";
    first=true;
    for(auto&[id,cnt]:player.item_usage_count){
        if(!first) js_data<<",";
        first=false;
        string name=ITEMS.count(id)?ITEMS.at(id)->name:id;
        js_data<<"'"<<name<<"'";
    }
    js_data << "];const itemData=[";
    first=true;
    for(auto&[id,cnt]:player.item_usage_count){
        if(!first) js_data<<",";
        first=false;
        js_data<<cnt;
    }
    js_data << "];";
    
    // ======== 生成HTML ========
    html << "<!DOCTYPE html>\n<html lang=\"zh-CN\">\n<head>\n"
         << "<meta charset=\"UTF-8\">\n"
         << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
         << "<title>校园RPG - 成长数据可视化面板</title>\n"
         << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n"
         << "<style>\n"
         << "*{margin:0;padding:0;box-sizing:border-box;}\n"
         << "body{font-family:'Microsoft YaHei','SimHei',sans-serif;background:linear-gradient(135deg,#1a1a2e 0%,#16213e 50%,#0f3460 100%);color:#e0e0e0;min-height:100vh;}\n"
         << ".header{text-align:center;padding:30px 20px 10px;}\n"
         << ".header h1{font-size:2em;background:linear-gradient(90deg,#00d2ff,#7b2ff7);-webkit-background-clip:text;-webkit-text-fill-color:transparent;margin-bottom:5px;}\n"
         << ".header .subtitle{color:#888;font-size:0.95em;}\n"
         << ".stats-bar{display:flex;justify-content:center;gap:30px;padding:15px;flex-wrap:wrap;}\n"
         << ".stat-card{background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:15px 25px;text-align:center;min-width:120px;backdrop-filter:blur(10px);}\n"
         << ".stat-card .value{font-size:2em;font-weight:bold;background:linear-gradient(90deg,#f7971e,#ffd200);-webkit-background-clip:text;-webkit-text-fill-color:transparent;}\n"
         << ".stat-card .label{color:#888;font-size:0.85em;margin-top:5px;}\n"
         << ".charts-grid{display:grid;grid-template-columns:1fr 1fr;gap:20px;padding:20px;max-width:1400px;margin:0 auto;}\n"
         << "@media(max-width:900px){.charts-grid{grid-template-columns:1fr;}}\n"
         << ".chart-container{background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.08);border-radius:16px;padding:20px;backdrop-filter:blur(10px);}\n"
         << ".chart-container h3{text-align:center;color:#ccc;margin-bottom:10px;font-size:1.1em;}\n"
         << ".chart-wrapper{position:relative;height:300px;}\n"
         << ".footer{text-align:center;padding:20px;color:#555;font-size:0.8em;}\n"
         << ".empty-msg{text-align:center;color:#666;padding:40px;font-size:1.1em;}\n"
         << "</style>\n</head>\n<body>\n";
    
    // Header
    html << "<div class=\"header\">\n<h1>📊 成长数据可视化面板</h1>\n"
         << "<div class=\"subtitle\">角色: "<<player.name
         <<" | 方向: "<<(player.track=="arts"?"文科":"理科")
         <<" | Lv."<<player.level<<"</div>\n</div>\n";
    
    // Stats bar
    html << "<div class=\"stats-bar\">\n"
         << "<div class=\"stat-card\"><div class=\"value\">"<<player.level<<"</div><div class=\"label\">当前等级</div></div>\n"
         << "<div class=\"stat-card\"><div class=\"value\">"<<player.max_hp<<"</div><div class=\"label\">最大HP</div></div>\n"
         << "<div class=\"stat-card\"><div class=\"value\">"<<player.atk()<<"</div><div class=\"label\">学识(ATK)</div></div>\n"
         << "<div class=\"stat-card\"><div class=\"value\">"<<player.dfs()<<"</div><div class=\"label\">抗压(DFS)</div></div>\n"
         << "<div class=\"stat-card\"><div class=\"value\">"<<player.total_battles<<"</div><div class=\"label\">总战斗</div></div>\n"
         << "<div class=\"stat-card\"><div class=\"value\">"<<player.total_wins<<"</div><div class=\"label\">胜利</div></div>\n"
         << "</div>\n";
    
    // Charts grid
    html << "<div class=\"charts-grid\">\n";
    
    // Chart 1: Growth Curve
    html << "<div class=\"chart-container\">\n<h3>📈 角色成长曲线</h3>\n<div class=\"chart-wrapper\"><canvas id=\"growthChart\"></canvas></div>\n</div>\n";
    
    // Chart 2: Radar
    html << "<div class=\"chart-container\">\n<h3>🎯 学科能力雷达图</h3>\n<div class=\"chart-wrapper\"><canvas id=\"radarChart\"></canvas></div>\n</div>\n";
    
    // Chart 3: Stage History
    html << "<div class=\"chart-container\">\n<h3>🏆 关卡通关历史</h3>\n<div class=\"chart-wrapper\"><canvas id=\"stageChart\"></canvas></div>\n</div>\n";
    
    // Chart 4: Item Frequency
    html << "<div class=\"chart-container\">\n<h3>💊 物品使用频率</h3>\n<div class=\"chart-wrapper\"><canvas id=\"itemChart\"></canvas></div>\n</div>\n";
    
    html << "</div>\n<div class=\"footer\">校园RPG - 成长数据可视化面板 | 数据实时更新</div>\n";
    
    // JavaScript
    html << "<script>\n" << js_data.str() << "\n";
    html << R"(
// Chart.js 全局配置 - 深色主题
Chart.defaults.color='#aaa';
Chart.defaults.borderColor='rgba(255,255,255,0.1)';

// 图表1: 成长曲线 (折线图)
if(growthLabels.length>0){
new Chart(document.getElementById('growthChart'),{
type:'line',
data:{
labels:growthLabels,
datasets:[
{label:'最大HP',data:growthHP,borderColor:'rgb(0,210,255)',backgroundColor:'rgba(0,210,255,0.1)',tension:0.3,fill:true,pointRadius:4,pointHoverRadius:7},
{label:'学识(ATK)',data:growthATK,borderColor:'rgb(255,152,0)',backgroundColor:'rgba(255,152,0,0.1)',tension:0.3,fill:true,pointRadius:4,pointHoverRadius:7},
{label:'抗压(DFS)',data:growthDFS,borderColor:'rgb(76,175,80)',backgroundColor:'rgba(76,175,80,0.1)',tension:0.3,fill:true,pointRadius:4,pointHoverRadius:7}
]},
options:{
responsive:true,maintainAspectRatio:false,
plugins:{legend:{labels:{usePointStyle:true,padding:20}}},
scales:{x:{title:{display:true,text:'等级'},grid:{color:'rgba(255,255,255,0.05)'}},y:{title:{display:true,text:'属性值'},grid:{color:'rgba(255,255,255,0.05)'}}}
}
});
}else{
document.getElementById('growthChart').parentElement.innerHTML='<div class="empty-msg">暂无成长数据<br><small>升级后将自动记录</small></div>';
}

// 图表2: 学科雷达图
new Chart(document.getElementById('radarChart'),{
type:'radar',
data:{
labels:radarLabels,
datasets:[{
label:'能力值',
data:radarData,
backgroundColor:'rgba(123,47,247,0.25)',
borderColor:'rgba(123,47,247,0.8)',
borderWidth:2,
pointBackgroundColor:'rgba(123,47,247,1)',
pointRadius:5,
pointHoverRadius:7
}]
},
options:{
responsive:true,maintainAspectRatio:false,
plugins:{legend:{display:false}},
scales:{r:{min:0,max:100,ticks:{stepSize:20,backdropColor:'transparent',color:'#666'},grid:{color:'rgba(255,255,255,0.1)'},pointLabels:{color:'#aaa',font:{size:12}}}}
}
});

// 图表3: 关卡通关历史 (柱状图)
if(stageLabels.length>0){
new Chart(document.getElementById('stageChart'),{
type:'bar',
data:{
labels:stageLabels,
datasets:[{
label:'通关状态',
data:stageData,
backgroundColor:stageBg,
borderColor:stageBg,
borderRadius:6,
barThickness:30
}]
},
options:{
responsive:true,maintainAspectRatio:false,
indexAxis:'x',
plugins:{
legend:{display:false},
tooltip:{callbacks:{label:function(c){return c.raw>=5?'✅ 通关':'❌ 失败';}}}
},
scales:{
x:{grid:{color:'rgba(255,255,255,0.05)'}},
y:{min:0,max:6,ticks:{stepSize:1,callback:function(v){return v>=5?'通关':v>=1?'失败':'';}},grid:{color:'rgba(255,255,255,0.05)'}}
}
}
});
}else{
document.getElementById('stageChart').parentElement.innerHTML='<div class="empty-msg">暂无关卡记录<br><small>挑战关卡后将自动记录</small></div>';
}

// 图表4: 物品使用频率 (横向柱状图)
if(itemLabels.length>0){
new Chart(document.getElementById('itemChart'),{
type:'bar',
data:{
labels:itemLabels,
datasets:[{
label:'使用次数',
data:itemData,
backgroundColor:'rgba(0,210,255,0.6)',
borderColor:'rgba(0,210,255,0.9)',
borderWidth:1,
borderRadius:4
}]
},
options:{
responsive:true,maintainAspectRatio:false,
indexAxis:'y',
plugins:{legend:{display:false}},
scales:{
x:{title:{display:true,text:'使用次数'},grid:{color:'rgba(255,255,255,0.05)'},ticks:{stepSize:1}},
y:{grid:{display:false}}
}
}
});
}else{
document.getElementById('itemChart').parentElement.innerHTML='<div class="empty-msg">暂无物品使用记录<br><small>在战斗中使用药品后将自动记录</small></div>';
}
</script>
</body></html>)";
    
    return html.str();
}

// 【新增】跨平台打开浏览器
void open_in_browser(const string& filepath) {
#ifdef _WIN32
    string cmd = "start \"\" \"" + filepath + "\"";
    system(cmd.c_str());
#elif __APPLE__
    string cmd = "open \"" + filepath + "\"";
    system(cmd.c_str());
#else
    string cmd = "xdg-open \"" + filepath + "\"";
    system(cmd.c_str());
#endif
}
