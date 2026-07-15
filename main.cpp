
// 文件: main.cpp
// 功能: 程序入口 + 游戏初始化 + 主菜单循环
// 包含: init_game_data(数据初始化)、print_title(标题画面)、
//        print_rules(新手引导)、create_character(角色创建)、
//        load_or_create(存档管理)、main_menu(主菜单)、main(入口)
// 设计: 模块化函数组织, 每个函数职责单一
#include "campus_rpg.h"

// init_game_data: 初始化全部游戏数据(一次性注册)
// 【考点2 物品设计】初始化三种物品(课本/药品/装备)和多种敌人(含Boss)
// 【考点4.5 战斗系统】设计3种以上敌人: 9门学科难题 + 3个Boss(期中/期末/高考)
// 【考点4.3 商店系统】4个品质等级货架(绿/蓝/紫/金) + 药房
// 调用时机: 每次程序启动时由main()调用一次
void init_game_data() {
    // 物品类型一: 课本(Book) - 提升学科掌握程度
    // 9门学科各对应一本课本, 售价50金币
    // 使用效果: 从未掌握→了解, 从了解→精通
    // 不在商店出售, 需通过任务/关卡获得
    // --- Books (not sold in shop, obtained elsewhere) ---
    ITEMS["book_chinese"]=new Book("book_chinese","语文课本","语文教材，掌握中华文化的精髓",50,"语文");
    ITEMS["book_math"]=new Book("book_math","数学课本","数学教材，逻辑与计算的基石",50,"数学");
    ITEMS["book_english"]=new Book("book_english","英语课本","英语教材，国际交流的桥梁",50,"英语");
    ITEMS["book_physics"]=new Book("book_physics","物理课本","物理教材，探索万物规律",50,"物理");
    ITEMS["book_chemistry"]=new Book("book_chemistry","化学课本","化学教材，物质变化的奥秘",50,"化学");
    ITEMS["book_biology"]=new Book("book_biology","生物课本","生物教材，生命的奇迹",50,"生物");
    ITEMS["book_history"]=new Book("book_history","历史课本","历史教材，以史为鉴",50,"历史");
    ITEMS["book_politics"]=new Book("book_politics","政治课本","政治教材，公民的必修课",50,"政治");
    ITEMS["book_geography"]=new Book("book_geography","地理课本","地理教材，世界之大",50,"地理");

    // 物品类型二: 药品(Potion) - 恢复HP/临时提升属性
    // 药品在战斗中和背包中均可使用
    // 分类: 内科(回血) / 强化科(提升属性)
    // 从医院药房购买
    // --- Potions (sold in pharmacy) ---
    ITEMS["potion_small_hp"]=new Potion("potion_small_hp","创可贴","恢复30点HP",30,30);
    ITEMS["potion_big_hp"]=new Potion("potion_big_hp","碘伏","恢复80点HP",80,80);
    ITEMS["potion_full_hp"]=new Potion("potion_full_hp","酒精","完全恢复HP",150,9999);
    ITEMS["potion_atk"]=new Potion("potion_atk","提神饮料","临时提升10点学识",40,0,10);
    ITEMS["potion_dfs"]=new Potion("potion_dfs","镇定剂","临时提升10点抗压",40,0,0,10);
    ITEMS["potion_mega"]=new Potion("potion_mega","体能强化剂","临时提升15点学识和15点抗压",100,0,15,15);

    // ============================================
    // 物品类型三: 装备(Equipment) - 衣服类(clothes)和文具类(tool)
    // 每人可装备1件衣服+1件文具, 提供属性加成和特殊效果
    // 品质分级: 绿色(1级货架) → 蓝色(2级) → 紫色(3级) → 金色(4级)
    // 特殊效果类型: uniform_survive(校服守护)、pen_core(百分比伤害)、
    //   notebook(学科精通)、hope_heal(回合回血)、star_pen(元素伤害)
    // ============================================
    // --- Initial equipment ---
    ITEMS["equip_uniform"]=new Equipment("equip_uniform","校服","校服守护：受到致命伤害后血量变为1(仅一次)",60,"clothes",5,5,5,"",0,false,0,"","uniform_survive",1,"绿色");
    ITEMS["equip_notebook"]=new Equipment("equip_notebook","笔记本(普通)","普通的笔记本",50,"tool",10);

    // --- 学科笔记本 (紫色品质, shop level 3) ---
    ITEMS["equip_nb_chinese"]=new Equipment("equip_nb_chinese","语文笔记本","装备后语文学科变为精通",200,"tool",20,5,0,"语文",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_math"]=new Equipment("equip_nb_math","数学笔记本","装备后数学学科变为精通",200,"tool",20,5,0,"数学",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_english"]=new Equipment("equip_nb_english","英语笔记本","装备后英语学科变为精通",200,"tool",20,5,0,"英语",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_physics"]=new Equipment("equip_nb_physics","物理笔记本","装备后物理学科变为精通",200,"tool",20,5,0,"物理",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_chemistry"]=new Equipment("equip_nb_chemistry","化学笔记本","装备后化学学科变为精通",200,"tool",20,5,0,"化学",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_biology"]=new Equipment("equip_nb_biology","生物笔记本","装备后生物学科变为精通",200,"tool",20,5,0,"生物",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_history"]=new Equipment("equip_nb_history","历史笔记本","装备后历史学科变为精通",200,"tool",20,5,0,"历史",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_politics"]=new Equipment("equip_nb_politics","政治笔记本","装备后政治学科变为精通",200,"tool",20,5,0,"政治",0,false,0,"","notebook",3,"紫色");
    ITEMS["equip_nb_geography"]=new Equipment("equip_nb_geography","地理笔记本","装备后地理学科变为精通",200,"tool",20,5,0,"地理",0,false,0,"","notebook",3,"紫色");

    // --- 星辰笔 (金色品质, shop level 4) ---
    ITEMS["equip_star_pen"]=new Equipment("equip_star_pen","星辰笔","附带目标生命5%的真实伤害",380,"tool",35,12,0,"",0,false,0,"","pen_core",4,"金色",20);

    // --- 传奇学神校服 (金色品质, shop level 4) ---
    ITEMS["equip_hope_uniform"]=new Equipment("equip_hope_uniform","传奇学神校服","每回合恢复最大HP的10%，受到致命伤害后血量变为1(仅一次)",400,"clothes",30,25,50,"",0,false,0,"","uniform_survive",4,"金色",0,10);

    // --- Shop: 绿色装备 (level 1) ---
    ITEMS["equip_green_clothes1"]=new Equipment("equip_green_clothes1","新手校服","新手款校服，校服守护",80,"clothes",8,8,10,"",0,false,0,"","uniform_survive",1,"绿色");
    ITEMS["equip_green_tool1"]=new Equipment("equip_green_tool1","碳素笔","碳素笔，附带目标5%血量追加伤害",60,"tool",12,0,0,"",0,false,0,"","pen_core",1,"绿色");
    ITEMS["equip_green_boost"]=new Equipment("equip_green_boost","基础强化卷","学识+8, 抗压+5, HP+10",50,"tool",8,5,10,"",0,false,0,"","",1,"绿色");

    // --- Shop: 蓝色装备 (level 2) ---
    ITEMS["equip_blue_clothes1"]=new Equipment("equip_blue_clothes1","中游校服","中游款校服，校服守护",150,"clothes",15,12,20,"",0,false,0,"","uniform_survive",2,"蓝色");
    ITEMS["equip_blue_tool1"]=new Equipment("equip_blue_tool1","职业式笔","职业式用笔，附带目标5%血量追加伤害",120,"tool",20,5,0,"",0,false,0,"","pen_core",2,"蓝色");
    ITEMS["equip_blue_boost"]=new Equipment("equip_blue_boost","中级强化卷","学识+15, 抗压+10, HP+20",100,"tool",15,10,20,"",0,false,0,"","",2,"蓝色");

    // --- Shop: 紫色装备 (level 3) ---
    ITEMS["equip_purple_clothes1"]=new Equipment("equip_purple_clothes1","学霸校服","学霸级校服，校服守护",250,"clothes",22,18,35,"",0,false,0,"","uniform_survive",3,"紫色");
    ITEMS["equip_purple_tool1"]=new Equipment("equip_purple_tool1","钢笔","精制钢笔，附带目标5%血量追加伤害",220,"tool",28,8,0,"",0,false,0,"","pen_core",3,"紫色");
    ITEMS["equip_purple_boost"]=new Equipment("equip_purple_boost","高级强化卷","学识+25, 抗压+18, HP+35",200,"tool",25,18,35,"",0,false,0,"","",3,"紫色");

    // --- Shop: 金色装备 (level 4) ---
    ITEMS["equip_gold_boost"]=new Equipment("equip_gold_boost","终极强化卷","学识+40, 抗压+30, HP+60",350,"tool",40,30,60,"",0,false,0,"","",4,"金色");

    // ============================================
    // 商店货架索引: 按品质分为4级货架
    // SHOP_LEVEL1: 绿色品质(初级装备)
    // SHOP_LEVEL2: 蓝色品质(中级装备)
    // SHOP_LEVEL3: 紫色品质(高级装备+学科笔记本)
    // SHOP_LEVEL4: 金色品质(稀有装备+星辰笔+希望校服)
    // ============================================
    // --- Shop level assignments ---
    SHOP_LEVEL1 = {"equip_green_clothes1","equip_green_tool1","equip_green_boost"};
    SHOP_LEVEL2 = {"equip_blue_clothes1","equip_blue_tool1","equip_blue_boost"};
    SHOP_LEVEL3 = {"equip_purple_clothes1","equip_purple_tool1","equip_purple_boost","equip_nb_chinese","equip_nb_math","equip_nb_english","equip_nb_physics","equip_nb_chemistry","equip_nb_biology","equip_nb_history","equip_nb_politics","equip_nb_geography"};
    SHOP_LEVEL4 = {"equip_gold_boost","equip_star_pen","equip_hope_uniform"};

    // Combined shop items list (for compatibility with existing code like rest_point)
    SHOP_ITEMS = SHOP_LEVEL1;
    SHOP_ITEMS.insert(SHOP_ITEMS.end(), SHOP_LEVEL2.begin(), SHOP_LEVEL2.end());
    SHOP_ITEMS.insert(SHOP_ITEMS.end(), SHOP_LEVEL3.begin(), SHOP_LEVEL3.end());
    SHOP_ITEMS.insert(SHOP_ITEMS.end(), SHOP_LEVEL4.begin(), SHOP_LEVEL4.end());

    // --- Pharmacy items ---
    PHARMACY_ITEMS = {"potion_small_hp","potion_big_hp","potion_full_hp","potion_atk","potion_dfs","potion_mega"};

    // ============================================
    // 敌人设计: 9门学科难题 + 3个Boss
    // 学科难题: 单属性个体, 有基础学识/抗压/HP
    // Boss: 期中考试(aoe_attack)、期末考试(aoe+自愈)、高考冲刺(aoe+自愈+真实伤害)
    // 关卡中通过create_stage_enemy()按难度等级调整属性
    // ============================================
    // --- Enemies ---
    ENEMIES_MAP["enemy_chinese"]=Enemy("enemy_chinese","语文难题","语文",80,25,15,30,20);
    ENEMIES_MAP["enemy_math"]=Enemy("enemy_math","数学难题","数学",90,30,12,35,25);
    ENEMIES_MAP["enemy_english"]=Enemy("enemy_english","英语难题","英语",75,28,14,32,22);
    ENEMIES_MAP["enemy_physics"]=Enemy("enemy_physics","物理难题","物理",100,32,18,38,28);
    ENEMIES_MAP["enemy_chemistry"]=Enemy("enemy_chemistry","化学难题","化学",95,30,16,36,26);
    ENEMIES_MAP["enemy_biology"]=Enemy("enemy_biology","生物难题","生物",85,26,14,34,24);
    ENEMIES_MAP["enemy_history"]=Enemy("enemy_history","历史难题","历史",88,27,13,33,23);
    ENEMIES_MAP["enemy_politics"]=Enemy("enemy_politics","政治难题","政治",82,24,16,31,21);
    ENEMIES_MAP["enemy_geography"]=Enemy("enemy_geography","地理难题","地理",86,26,15,33,23);
    ENEMIES_MAP["boss_midterm"]=Enemy("boss_midterm","期中考试","综合",200,40,25,100,80,{"aoe_attack"},true);
    ENEMIES_MAP["boss_final"]=Enemy("boss_final","期末考试","综合",350,55,35,200,150,{"aoe_attack","heal_self"},true);
    ENEMIES_MAP["boss_entrance"]=Enemy("boss_entrance","高考冲刺","综合",500,70,50,400,300,{"aoe_attack","heal_self","true_damage"},true);

    for(auto& s:ALL_SUBJECTS){
        if(s=="语文") SUBJECT_ENEMIES[s]="enemy_chinese";
        else if(s=="数学") SUBJECT_ENEMIES[s]="enemy_math";
        else if(s=="英语") SUBJECT_ENEMIES[s]="enemy_english";
        else if(s=="物理") SUBJECT_ENEMIES[s]="enemy_physics";
        else if(s=="化学") SUBJECT_ENEMIES[s]="enemy_chemistry";
        else if(s=="生物") SUBJECT_ENEMIES[s]="enemy_biology";
        else if(s=="历史") SUBJECT_ENEMIES[s]="enemy_history";
        else if(s=="政治") SUBJECT_ENEMIES[s]="enemy_politics";
        else if(s=="地理") SUBJECT_ENEMIES[s]="enemy_geography";
    }

    // ============================================
    // 卡牌系统: 10种肉鸽卡牌, 在关卡间隙随机获得
    // 每张卡牌提供一种战斗buff, 同类型buff不可重复获取
    // 效果在整场关卡挑战中持续生效
    // ============================================
    // --- Cards ---
    CARDS={new Card1SubjectUpgrade(),new Card2PercentTrueDamage(),new Card3LifeStealPct(),
           new Card4DamageReduction(),new Card5EnergyReturn(),new Card6DefendImmune(),
           new Card7EnergyEfficiency(),new Card8SurviveCharge(),new Card9UltimateDebuff(),new Card10ReflectDamage()};

    // ============================================
    // 藏品(秘宝)系统: 6种永久增强效果
    // 在关卡通关时有30%概率随机获得未持有的藏品
    // 藏品效果永久生效(除月见八千代为可召唤)
    // ============================================
    // --- Treasures ---
    TREASURES={new Treasure1DfsBoost(),new Treasure2ExpGoldBoost(),
               new Treasure3ReviveTruePath(),new Treasure4AllyBoost(),
               new Treasure5RandomNotebook(),new Treasure6YachiyoSummon()};

    // ============================================
    // 任务系统: 6个日常任务
    // 类型: 扫地/帮助老师/完成作业/随堂测验/晨跑/自习
    // 奖励: 经验+金币, 部分任务附赠物品
    // ============================================
    // --- Quests ---
    QUESTS={
        Quest("quest_sweep","打扫教室","打扫教室卫生，锻炼责任感","sweep",20,10,"",1),
        Quest("quest_help_teacher","帮助老师","帮助老师批改作业，增进师生关系","help_teacher",25,15,"",1),
        Quest("quest_homework","完成作业","认真完成今天的家庭作业","homework",30,20,"",1),
        Quest("quest_quiz","随堂测验","进行一次随堂测验","quiz",50,40,"",1),
        Quest("quest_running","晨跑锻炼","早起晨跑，锻炼体魄","running",35,25,"",1),
        Quest("quest_study","自主学习","在图书馆自主学习","study",40,30,"",1)
    };
}

// ============================================
// Game Flow
// ============================================
// ============================================
// print_title: 打印ASCII艺术风格的标题画面
// 每次打开主菜单/创建角色时调用, 提升视觉体验
// 【考点7 人机交互】ASCII艺术标题, 清晰的界面设计
// ============================================
void print_title() {
    cout<<"\n  ========================================="<<endl;
    cout<<"     校园 RPG - 知识即力量"<<endl;
    cout<<"     Campus RPG - Knowledge is Power"<<endl;
    cout<<"  ========================================="<<endl;
}

// ============================================
// print_rules: 打印新手引导说明
// 包含: 游戏目标、文理选择说明、学科克制关系、战斗公式
// 玩家可在主菜单选"9"随时查看
// 【考点7 人机交互】新手引导 - 说明游戏规则/学科克制/战斗公式
// ============================================
void print_rules() {
    cout<<"\n  =================================================="<<endl;
    cout<<"                     新手引导"<<endl;
    cout<<"  -------------------------------------------------"<<endl;
    cout<<"   在这个校园RPG世界中，你需要："<<endl;
    cout<<"   学习学科知识，从「了解」到「精通」"<<endl;
    cout<<"   用学科知识击败各种'考试难题'"<<endl;
    cout<<"   赚取金币到商店购买装备和药品"<<endl;
    cout<<"   完成任务获得经验和奖励"<<endl;
    cout<<"   收集卡牌增强战斗能力"<<endl;
    cout<<"   挑战各大关卡(日常作业->期中->期末->高考)"<<endl;
    cout<<"  -------------------------------------------------"<<endl;
    cout<<"   【文理选择】"<<endl;
    cout<<"   文科(语文/英语/历史/政治/地理/数学) 自选1~6门精通"<<endl;
    cout<<"   理科(数学/物理/化学/生物/英语/语文) 自选1~6门精通"<<endl;
    cout<<"   非选择方向的科目自动设为「了解」"<<endl;
    cout<<"  -------------------------------------------------"<<endl;
    cout<<"   【学科克制】"<<endl;
    cout<<"   精通学科攻击 -> 造成2.0倍伤害，受到0.5倍伤害"<<endl;
    cout<<"   了解学科攻击 -> 造成0.5倍伤害，受到1.5倍伤害"<<endl;
    cout<<"   未掌握学科 -> 造成1.0倍伤害，受到1.0倍伤害"<<endl;
    cout<<"  -------------------------------------------------"<<endl;
    cout<<"   【战斗公式】"<<endl;
    cout<<"   普通攻击伤害 = 学识(ATK) - 抗压(DFS) (最小值1)"<<endl;
    cout<<"   技能伤害 = (学识 x 倍率) / 抗压"<<endl;
    cout<<"   真实伤害直接结算，不受任何减伤影响"<<endl;
    cout<<"  =================================================="<<endl;
}

// ============================================
// create_character: 交互式角色创建流程
// 步骤: ①输入姓名 → ②选择文/理方向 → ③从方向学科中选择3门精通
// 流程: 输入验证 → 学科分配 → 初始装备应用 → 返回Player对象
// 其他学科(对方方向)自动设为"了解"
// 【考点4.1 角色管理】创建角色: 输入姓名→选择文理方向→选择3门精通学科
// 【考点7 人机交互】交互式创建流程, 清晰的提示信息
// ============================================
Player create_character() {
    clear_screen();
    print_title();
    cout<<"  创建你的角色\n"<<endl;
    string name;
    cout<<"  请输入角色姓名: "; getline(cin,name);
    if(name.empty()) name="无名学生";

    cout<<"\n  请选择你的方向:"<<endl;
    cout<<"  1. 文科 (语文/英语/历史/政治/地理/数学)"<<endl;
    cout<<"  2. 理科 (数学/物理/化学/生物/英语/语文)"<<endl;
    string track; vector<string> main_subjs;
    while(true){
        cout<<"  选择(1/2): ";
        string tc; getline(cin,tc);
        if(tc=="1"){track="arts";main_subjs=ARTS;break;}
        else if(tc=="2"){track="sciences";main_subjs=SCIENCES;break;}
        cout<<"  请输入1或2。"<<endl;
    }

    cout<<"\n  从以下学科中选择1~6门精通:"<<endl;
    for(size_t i=0;i<main_subjs.size();i++) cout<<"  ["<<(i+1)<<"] "<<main_subjs[i]<<endl;
    cout<<"  输入1~6个编号(用空格分隔，如: 1 3 5)"<<endl;

    vector<string> specifics;
    while(true){
        cout<<"  选择: ";
        string line; getline(cin,line);
        istringstream iss(line);
        vector<int> idx; int x;
        while(iss>>x) idx.push_back(x-1);
        if(idx.size()>=1 && idx.size()<=6){
            bool ok=true;
            for(auto i:idx) if(i<0||i>=(int)main_subjs.size()){ok=false;break;}
            // 检查重复
            for(size_t a=0;a<idx.size()&&ok;a++)
                for(size_t b=a+1;b<idx.size();b++)
                    if(idx[a]==idx[b]){ok=false;break;}
            if(ok){
                for(auto i:idx) specifics.push_back(main_subjs[i]);
                break;
            }
        }
        cout<<"  请选择1~6门不同的学科。"<<endl;
    }

    Player player(name);
    player.setup_initial_subjects(track,specifics);
    player.apply_initial_equipment();

    cout<<"\n  角色创建成功！"<<endl;
    cout<<"  "<<player.name<<" - "<<(track=="arts"?"文科":"理科")<<" 学生"<<endl;
    cout<<"  精通: ";
    for(size_t i=0;i<specifics.size();i++){if(i>0)cout<<", ";cout<<specifics[i];}
    cout<<endl;
    return player;
}

// ============================================
// load_or_create: 存档检测与加载/新建决策
// 检查saves/save.json是否存在 → 存在则提示加载/新建 → 不存在则直接创建
// 新建角色会覆盖旧存档(二次确认)
// 【考点5 数据持久化】检测存档文件是否存在, 支持加载或新建
// 【考点4.1 角色管理】加载存档=角色读取功能
// ============================================
Player load_or_create() {
    ifstream check("saves/save.json");
    if(check.good()){check.close();
        cout<<"  检测到已有存档。"<<endl;
        cout<<"  1. 加载存档继续游戏"<<endl;
        cout<<"  2. 创建新角色（会覆盖旧存档）"<<endl;
        string ch; cout<<"  选择(1/2): "; getline(cin,ch);
        if(ch=="1"){
            try{Player p=Player::load("saves/save.json"); cout<<"\n  存档加载成功！欢迎回来，"<<p.name<<"！"<<endl;return p;}
            catch(...){cout<<"  加载失败"<<endl;}
        }else{
            cout<<"  确认创建新角色将覆盖旧存档。"<<endl;
            cout<<"  确认？(y/n): "; string conf; getline(cin,conf);
            if(conf!="y"&&conf!="Y") return load_or_create();
        }
    }
    return create_character();
}

// ============================================
// main_menu: 游戏主循环 - 总调度中心
// 功能: 创建所有子系统对象 → 显示角色状态 → 菜单选择 → 分派到子系统
// 子系统: InventorySystem(背包)、ShopSystem(商店+药房)、QuestSystem(任务)、
//         GrowthSystem(成长)、StageSystem(关卡+战斗)
// 循环: 持续运行直到用户选择"0"退出
// 【考点3.2 必须类设计】游戏管理类(主菜单)协调所有子系统
// 【考点7 人机交互】主菜单: 清晰菜单结构, 编号选择, 无效输入处理
// 【考点1.4 类间关联】组合所有子系统
// ============================================
void main_menu(Player& player) {
    InventorySystem inv(&player);   // 【考点1.4 类间关联】背包系统关联Player
    ShopSystem shop(&player);       // 【考点1.4 类间关联】商店系统关联Player
    QuestSystem quest(&player);    // 【考点1.4 类间关联】任务系统关联Player
    GrowthSystem growth(&player);  // 【考点1.4 类间关联】成长系统关联Player
    StageSystem stage(&player);    // 【考点1.4 类间关联】关卡系统关联Player
    AutoSaveSystem auto_save(&player);  // 【挑战2:多线程】后台自动存档系统
    auto_save.start();                  // 【挑战2:多线程】启动后台存档线程

    while(true){
        clear_screen();
        print_title();
        player.display();
        cout<<"\n  主菜单"<<endl;
        cout<<"  --------------------------------------"<<endl;
        cout<<"  1. 任务系统    | 2. 商店购物"<<endl;
        cout<<"  3. 背包        | 4. 医院药房"<<endl;
        cout<<"  5. 关卡挑战    | 6. 成长系统"<<endl;
        cout<<"  7. 休息恢复    | 8. 保存游戏"<<endl;
        cout<<"  9. 游戏说明    | 10. 测试游戏"<<endl;
        cout<<"  11. 成长可视化  | 0. 退出游戏"<<endl;
        cout<<"  --------------------------------------"<<endl;
        string ch; cout<<"  请选择: "; getline(cin,ch);
        if(ch=="1") quest.run();           // 【考点4.4 任务系统】入口
        else         if(ch=="2") shop.run();// 【考点4.3 商店系统】入口
        else if(ch=="3") inv.run();         // 【考点4.2 背包管理】入口
        else if(ch=="4") shop.pharmacy();   // 【考点4.3 商店系统】药房入口
        else if(ch=="5") stage.run();       // 【考点4.5 战斗系统】关卡入口
        else if(ch=="6") growth.run();      // 【考点4.6 等级成长】入口
        else if(ch=="7"){
            player.hp=(min)(player.max_hp,player.hp+50);
            cout<<"\n  休息恢复了50点HP。"<<endl;
        }
        else if(ch=="8"){                   // 【考点5 数据持久化】保存入口
#ifdef _WIN32
            system("mkdir saves 2>nul");
#else
            system("mkdir -p saves");
#endif
            player.save("saves/save.json");  // 【考点5】调用保存
            cout<<"\n  游戏已保存！"<<endl;
        }
        else if(ch=="9") print_rules();
        else if(ch=="10"){
            // 测试游戏：角色升至100级，获得10000G金币，全学科精通
            while(player.level < 100){
                player.level_up();
            }
            player.gold += 10000;
            player.hp = player.max_hp;
            for(auto&[s,m]:player.subjects) m="精通";
            cout<<"\n  [测试模式] 角色已升至Lv.100，获得10000G金币，全学科精通！"<<endl;
        }
        else if(ch=="11"){  // 【新增】成长数据可视化面板
            string filepath = "saves/growth_dashboard.html";
#ifdef _WIN32
            system("mkdir saves 2>nul");
#else
            system("mkdir -p saves");
#endif
            ofstream f(filepath);
            f<<generate_dashboard_html(player);
            f.close();
            cout<<"\n  正在生成成长可视化面板..."<<endl;
            open_in_browser(filepath);
            cout<<"  已在浏览器中打开可视化面板！"<<endl;
            cout<<"  文件位置: "<<filepath<<endl;
        }
        else if(ch=="0"){
            auto_save.stop();  // 【挑战2:多线程】停止后台存档线程
            cout<<"\n  再见，"<<player.name<<"！"<<endl;exit(0);}
        else cout<<"  无效选择！"<<endl;     // 【考点7 人机交互】错误输入提示
        pause_msg();
    }
}

// ============================================
// main: 程序入口点
// 流程: ①设置控制台UTF-8编码(Windows) → ②初始化游戏数据 →
//       ③显示标题 → ④加载/创建角色 → ⑤进入主菜单循环
// 【程序入口】初始化游戏数据 → 加载或创建角色 → 进入主菜单
// ============================================
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);   // 【考点7 人机交互】设置控制台编码支持中文
#endif
    init_game_data();            // 初始化所有游戏数据
    clear_screen();
    print_title();
    Player player = load_or_create();  // 【考点5 数据持久化】存档检测与加载
    main_menu(player);                  // 进入主循环
    return 0;
}
