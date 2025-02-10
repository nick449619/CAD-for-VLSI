#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unordered_set>
#include <cstdlib> // For rand() and srand()
#include <ctime>   // For time()

using namespace std;

struct Dimension {
    double width;
    double height;
    int col;
    int row;
};

class Block; // 前置聲明

class Net {
public:
    string name;
    vector<Block*> blocks;

    Net(){};
    void add_block(Block* block); 
};

class Block {
public:
    string name; // BB0
    vector<Net*> nets;
    vector<Dimension*> dimensions;  // (width, heighr, col, row)
    int chosenBlock;
    bool isgroupblock = false;  
    vector<string> groupedBlocksName; // MM0 MM1
    bool isSymmetry = false;
    double center_x;        // for HPWL
    double center_y;        // for HPWL
    Block(){};

    void add_net(Net* net);
    void add_nets(const vector<Net*>& newNets); 
    void add_dimensions(Dimension* dim);

    int choose_dim_by_area();       // for B* tree
    int choose_dim_by_ratio();      // for B* tree
};

// 實現 Net::add_block 函式
void Net::add_block(Block* block) {
    // 檢查是否已經存在相同的 block name
    bool exists = false;
    for (Block* existingBlock : blocks) {
        if (existingBlock->name == block->name) {
            exists = true;
            break;
        }
    }
    // 如果不存在相同的 block name，則將其加入
    if (!exists) {
        blocks.push_back(block);
    }
}

// 實現 Block::add_net 函式
void Block::add_net(Net* net) {
    nets.push_back(net);
}

// 實現 Block::add_nets 函式
void Block::add_nets(const vector<Net*>& newNets) {
    for (Net* newNet : newNets) {
        // 檢查是否已經存在相同的 net
        bool exists = false;
        for (Net* existingNet : nets) {
            if (existingNet->name == newNet->name) { // 檢查名稱是否相同
                exists = true;
                break;
            }
        }
        // 如果不存在相同的 net，則將其加入
        if (!exists) {
            nets.push_back(newNet);
        }
    }
}

// 實現 Block::add_dimensions 函式
void Block::add_dimensions(Dimension* dim) {
    dimensions.push_back(dim);
}

// 實現 Block::choose_dim_by_area 函式
int Block::choose_dim_by_area() {
    int choose_dim = 0;
    double min_area = dimensions[0]->width * dimensions[0]->height;
    int vector_number = 0; // 追蹤最小面積的維度索引
    // 迭代 dimensions 向量找出最小面積
    for (size_t i = 1; i < dimensions.size(); ++i) {
        double area = dimensions[i]->width * dimensions[i]->height;
        if (area < min_area) {
            min_area = area;
            vector_number = i;
        }
    }
    // 返回最小面積的維度索引
    return vector_number;
}

int Block::choose_dim_by_ratio() {
    int chosen_dim = 0;
    double max_ratio = min(dimensions[0]->width, dimensions[0]->height) / max(dimensions[0]->width, dimensions[0]->height);
    
    // 迭代 dimensions 向量找出長寬比例最大的維度
    for (size_t i = 1; i < dimensions.size(); ++i) {
        double ratio = min(dimensions[i]->width, dimensions[i]->height) / max(dimensions[i]->width, dimensions[i]->height);
        if (ratio > max_ratio) {
            max_ratio = ratio;
            chosen_dim = i;
        }
    }
    
    // 返回長寬比例最大的維度索引
    return chosen_dim;
}


class Symmetry {
public:
    string name;
    double x;   //x座標
    vector<Block*> blocks;
    Symmetry(){};
    bool has_two_block = false;
    void add_block(Block* block){
        blocks.push_back(block);
    }
};

//global variable
vector<Net*> my_nets;
vector<Block*> my_blocks;
vector<Symmetry*> my_symmetrys;

void parseNetlist(ifstream& netlistFile) {
    string line;
    string word;

    vector<string> words; // 定義 words 向量
    vector<string> defineNets;
    bool Firstline = true;
    if (netlistFile.is_open()) { 
        while (getline(netlistFile, line)) {  
            // 處理第一行 
            // 處理後面
            int count = 1;
            if (line.at(0) != 'M' || count >= 4) continue;
            istringstream ss(line);
            words.clear(); // 清空 words 向量
            while(ss >> word){
                words.push_back(word);       
            }
            Block* blockTemp = new Block(); // 使用動態配置的 Block 物件指標
            blockTemp->name = words[0];
            for (int i = 1; i <= 3; ++i) {
                if (find(defineNets.begin(), defineNets.end(), words[i]) != defineNets.end()) {
                    continue;
                }
                Net* netTemp = new Net(); // 使用動態配置的 Net 物件指標
                netTemp->name = words[i];
                blockTemp->add_net(netTemp);
                
                // 檢查是否已經有這個 net
                bool found = false;
                for (Net* net : my_nets) {
                    if (net->name == netTemp->name) {
                        // 檢查 block 是否已經存在於 net 的 blocks 中
                        bool blockFound = false;
                        for (Block* block : net->blocks) {
                            if (block->name == blockTemp->name) {
                                blockFound = true;
                                break;
                            }
                        }
                        if (!blockFound) {
                            net->add_block(blockTemp);
                        }
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    netTemp->add_block(blockTemp);
                    my_nets.push_back(netTemp);
                }
            }
            my_blocks.push_back(blockTemp); 
            count++;
        }
        netlistFile.close();
    }
}

Block* findBlockByName(const string& blockName) {
    for (Block* block : my_blocks) {
        if (block->name == blockName) {
            return block;
        }
    }
    return nullptr;
}

Net* findNetByname(const string& netName){
    for (Net* net : my_nets) {
        if (net->name == netName) {
            return net;
        }
    }
    return nullptr;
}

void processDimensions(Block* block, const vector<string>& words) {
    double width, height;
    int col, row;
    int index = -1;

    // 找到第一個包含維度資訊的字串
    for (int i = 0; i < words.size(); ++i) {
        if (words[i].front() == '(') {
            index = i;
            break;
        }
    }
    if (index == -1) {
        return;
    }
    // 從找到的位置開始解析維度資訊
    for (; index < words.size() - 3; index += 4) {
        // 解析 width
        width = stod(words[index].substr(1));
        // 解析 height
        height = stod(words[index + 1]);
        // 解析 col
        col = stoi(words[index + 2]);
        // 解析 row，移除結尾的 ')' 字元
        row = stoi(words[index + 3].substr(0, words[index + 3].size() - 1));
        // 創建 Dimension 物件並添加到 Block 中
        Dimension* dim = new Dimension();
        dim->width = width;
        dim->height = height;
        dim->col = col;
        dim->row = row;
        block->add_dimensions(dim);
    }
}

void parseBuildingBlock(ifstream& buildingBlockFile) {
    string line;
    if (buildingBlockFile.is_open()) {
        while (getline(buildingBlockFile, line)) {
            istringstream ss(line);
            string word;
            vector<string> words;
            while (ss >> word) {
                words.push_back(word);
            }
            if (words[0][0] == 'B') {
                // 如果是 B 開頭，則
                // 1. 更新 my_blocks, 把要 group 起來的　block 刪掉, 然後把新的 group_block push 進來
                // 2. 更新 my_nets, 把 my_nets 裡的 block_vector  
                Block* groupBlock = new Block();
                groupBlock->name = words[0];
                processDimensions(groupBlock, words);
                for (size_t i = 1; i < words.size(); ++i) {
                    Block* block = findBlockByName(words[i]); // 找到對應的 Block
                    if (block) {
                        groupBlock->add_nets(block->nets); // 添加 Block 的 nets 到 groupBlock
                        groupBlock->isgroupblock = true;
                        groupBlock->groupedBlocksName.push_back(words[i]);// 尋找要刪除的 block
                        my_blocks.erase(remove_if(my_blocks.begin(), my_blocks.end(),[block](const Block* ptr) { return ptr == block; }), my_blocks.end());
                        // 更新 my_nets
                        for (Net* net : my_nets) {
                            for (auto it = net->blocks.begin(); it != net->blocks.end(); ) {
                                if (*it == block) {
                                    // 如果找到了要刪除的 block，使用 erase 刪除它
                                    it = net->blocks.erase(it);
                                    net->add_block(groupBlock);
                                } else {
                                    ++it;
                                }
                            }
                        }

                    }
                }
                my_blocks.push_back(groupBlock);
            }
                
                // 維度
            else if (words[0][0] == 'M') {
                Block* block = findBlockByName(words[0]);
                processDimensions(block, words);
            }
        }      
        buildingBlockFile.close();
    }
}

void parseSymmetry(ifstream& symmetryFile) {
    string line;
    string word;
    vector<string> words;
    if (symmetryFile.is_open()) {
        while (getline(symmetryFile, line)) {
            istringstream ss(line);
            words.clear();
            while (ss >> word) {
                words.push_back(word);
            }
            if (words.size() >= 2) { // 確保至少有兩個單詞
                Symmetry* symmetryTemp = new Symmetry();
                symmetryTemp->name = words[0];
                for (size_t i = 1; i < words.size(); ++i) {
                    Block* symmetryBlock = findBlockByName(words[i]);
                    symmetryBlock->isSymmetry = true;
                    symmetryTemp->add_block(symmetryBlock);
                    if(i==2){
                        symmetryTemp->has_two_block = true;
                    }
                }
                my_symmetrys.push_back(symmetryTemp);
            } 
        }
        symmetryFile.close();
    }
}

class BStarTree {
public: 
    double chip_width = 0;
    double chip_height = 0;
    vector<string> placement;

private:
    struct BStarTreeNode {
        Block* data;
        BStarTreeNode* left;
        BStarTreeNode* right;

        BStarTreeNode(Block* val) : data(val), left(nullptr), right(nullptr) {}
    };

    struct yContourNode {
        double x0;
        double xf;
        double y;
        yContourNode* prev;
        yContourNode* next;
    };

    BStarTreeNode* root;
    unordered_set<Block*> insertedBlocks;
    yContourNode* head = new yContourNode {0, 100000000, 0, nullptr, nullptr};

    void preorderTraversal(BStarTreeNode* root, double place_x) {
        if (root == nullptr)
            return;

        // 選擇最佳維度
        int index = root->data->choose_dim_by_ratio();
        Dimension* best_dim = root->data->dimensions[index];
        double blockheight = best_dim->height;
        double place_y = my_yContour(head, place_x, best_dim->width);
        insertNode(head, place_x, best_dim->width, blockheight);
        root->data->center_x = place_x + best_dim->width/2;
        root->data->center_y = place_y + best_dim->height/2;
        // 將當前節點的位置信息寫入 outputFile
        string outputString = root->data->name;
        for(const auto device : root->data->groupedBlocksName) {
            outputString += " " + device;
        }
        outputString += " " + to_string(place_x) + " " + to_string(place_y) + " ";
        outputString += "(" + to_string(best_dim->width) + " " + to_string(best_dim->height) + " " + to_string(best_dim->col) + " " + to_string(best_dim->row) + ")";
        
        chip_width = max(chip_width, place_x + best_dim->width);
        chip_height = max(chip_height, place_y + best_dim->height);

        // 将输出字符串存入placement中
        placement.push_back(outputString);

        // 遞迴處理左子樹，更新左子樹的 parent_x 和 parent_y
        preorderTraversal(root->left, place_x + best_dim->width);
        // 遞迴處理右子樹，更新右子樹的 parent_x 和 parent_y
        preorderTraversal(root->right, place_x);
    }

    // double my_yContour(yContourNode* head, double place_x, double blockwidth);
    void insertNode(yContourNode*& head, double place_x, double blockwidth, double height) {
        yContourNode* current = head;
        yContourNode* startOverlap = nullptr; // 堆疊起始 x 座標的節點
        yContourNode* endOverlap = nullptr;   // 堆疊終點 x 座標的節點

        // 找到新節點Start堆疊
        while (current != nullptr) {
            if(place_x >= current->x0 && place_x < current->xf){
                break;
            }
            current = current->next;
        }

        if (current == nullptr) {
            return;
        }
        startOverlap = current; // 設置起始堆疊節點

        // 找到新節點end堆疊
        while(current != nullptr){
            if(place_x + blockwidth >= current->x0 && place_x + blockwidth < current->xf){
                break;
            }
            current = current->next;
        }
        if (current == nullptr) {
            return;
        }
        endOverlap = current; // 設置結束堆疊節點
        double update_y = my_yContour(head, place_x, blockwidth) + height; 
        yContourNode* newNode = new yContourNode{place_x, place_x + blockwidth, update_y, startOverlap->prev, endOverlap};
        
        // 擺右邊或是擺上面小的
        if(startOverlap == endOverlap && place_x != 0){
            (startOverlap->prev)->next = newNode;       // 差在這
            if (endOverlap != nullptr) {
                endOverlap->x0 = place_x + blockwidth;
                endOverlap->prev = newNode;
            }
        }
            // 擺在最左邊
        else if(startOverlap == endOverlap && place_x == 0){
            if (endOverlap != nullptr) {
                endOverlap->x0 = place_x + blockwidth;
                endOverlap->prev = newNode;
            }
            head = newNode;                     //差在這
        }  
        
        // 擺上面大, 需要做 delete
        else if(startOverlap != endOverlap && place_x == 0){

            // 從 start 開始刪到 end->prev
            yContourNode* temp = startOverlap;
            while(temp != endOverlap->prev){
                temp = temp->next;
                delete temp;
            }

            if (endOverlap != nullptr) {
                endOverlap->x0 = place_x + blockwidth;
                endOverlap->prev = newNode;
            }
            head = newNode;
        }
        else { //startOverlap != endOverlap && place_x != 0
            yContourNode* temp = startOverlap;
            while(temp != endOverlap->prev){
                temp = temp->next;
                delete temp;
            }

            (startOverlap->prev)->next = newNode; 
            if(startOverlap->prev == nullptr){
                (startOverlap->prev)->next = newNode;
            }  

            if (endOverlap != nullptr) {
                endOverlap->x0 = place_x + blockwidth;
                endOverlap->prev = newNode;
            }
        }
        return;
    }
    double my_yContour(yContourNode* head, double place_x, double blockwidth) {
        double max_y = 0.0; // 最大高度初始化为0
        yContourNode* current = head;
        
        // 找到 place_x 所在的 linked list 的节点
        while (current != nullptr) {
            if (current->x0 <= place_x && current->xf > place_x) {
                break;
            }
            current = current->next; 
        }

        if (current == nullptr) {
            // 如果找不到 place_x 所在的节点，直接返回0
            return max_y;
        }

        // 找到 place_x + blockwidth 所在的 linked list 的节点
        while (current != nullptr ) {
            if (current->x0 < place_x + blockwidth && current->xf >= place_x + blockwidth) {
                // 找到范围内的节点后比较其 y 座标
                max_y = max(max_y, current->y);
                break;
            }
            max_y = max(max_y, current->y);
            current = current->next;
        }

        return max_y;
    }

public:
    BStarTree() : root(nullptr) {}

    void insertBlock(Block* block) {
        root = insertBlock(root, block);
    }

    void preorderTraversal() {
        preorderTraversal(root, 0);
    }

    void buildTree(vector<Block*>& blocks) {
        for (const auto block : blocks) {
            if (insertedBlocks.find(block) == insertedBlocks.end()) {
                insertBlock(block);
                insertedBlocks.insert(block);
            }
        }
    }

private:
    BStarTreeNode* insertBlock(BStarTreeNode* root, Block* block) {
        if(block->isSymmetry){
            return root;
        }

        if (root == nullptr) {
            return new BStarTreeNode(block);
        }
        int randomNum = rand() % 4; // rand() generates a pseudo-random integer
        // Insert based on probability
        if (randomNum < 3) { // 3/4 probability to insert on the left
            root->left = insertBlock(root->left, block);
        }
        else { // 1/4 probability to insert on the right
            root->right = insertBlock(root->right, block);
        }
        return root;
    }
};

BStarTree build_tree_by_net(){
    BStarTree tree;
    for(const auto net:my_nets){
        tree.buildTree(net->blocks);
    }

    tree.preorderTraversal();
    return tree;
}

void placeSymmetry(vector<string>& placement, double width, double height){
    double place_x;
    double max_blockwidth = 0;
    double place_y = 0;
    double sym_axis;
    for(const auto symmetry: my_symmetrys){
        double sumWidth = 0;
        for(const auto block: symmetry->blocks){
            int index = block->choose_dim_by_ratio();
            Dimension* best_dim = block->dimensions[index];
            sumWidth += best_dim->width;
        }
        max_blockwidth = max(max_blockwidth, sumWidth);
    }

    sym_axis = width + max_blockwidth/2;

    for(const auto symmetry: my_symmetrys){
        bool theBlockisFirst = true;
        double update_height;
        for(const auto block: symmetry->blocks){
            int index = block->choose_dim_by_ratio();
            Dimension* best_dim = block->dimensions[index];
            
            if(!symmetry->has_two_block){
                place_x = sym_axis-(best_dim->width)/2;
            }
            else if(theBlockisFirst && symmetry->has_two_block){
                place_x = sym_axis-(best_dim->width);
                theBlockisFirst = false;
            }
            // 有兩個 block 的第二個
            else{
                place_x = sym_axis;
            }

            block->center_x = place_x + best_dim->width/2;
            block->center_y = place_y + best_dim->height/2;
            
            // 存成字串
            string outputString = block->name;
            for(const auto device : block->groupedBlocksName) {
                outputString += " " + device;
            }
            outputString += " " + to_string(place_x) + " " + to_string(place_y) + " ";
            outputString += "(" + to_string(best_dim->width) + " " + to_string(best_dim->height) + " " + to_string(best_dim->col) + " " + to_string(best_dim->row) + ")";
            
            placement.push_back(outputString);
            update_height = best_dim->height;
        }
        place_y = place_y + update_height;
    }
    width = width + max_blockwidth;
    height = max(height, place_y);
    placement.push_back(to_string(height*width));
    placement.push_back(to_string(width) + " " +to_string(height));
}

double cal_HPWL(){
    double sum = 0;
    for(const auto net:my_nets){
    double min_x = 999999999;
    double max_x = 0;
    double min_y = 999999999; 
    double max_y = 0;
        for(const auto block : net->blocks){
            
            min_x = min(min_x, block->center_x);
            max_x = max(max_x, block->center_x);
            min_y = min(min_y, block->center_y);
            max_y = max(max_y, block->center_y);            
        }
        sum += (max_x - min_x) + (max_y - min_y);
    }
    //cout << sum << endl;
    return sum;
}

void generateOutput(vector<string>& placement, ofstream& outputfile){
    size_t size = placement.size();

    // 如果向量中的字符串少于两个，则直接输出整个向量
    if (size <= 2) {
        for (const auto& item : placement) {
            outputfile << item << endl;
        }
        return;
    }

    // 输出向量中的倒数两个字符串
    outputfile << cal_HPWL() << endl;
    outputfile << placement[size - 2] << endl;
    outputfile << placement[size - 1] << endl;

    for (size_t i = 0; i < size - 2; ++i) {
        outputfile << placement[i] << endl;
    }
}

int main(int argc, char *argv[]) {
    // 檢查命令行的參數數量, argc為命令數量, argv為字串的陣列
    if ( argc != 6 ){
        cerr << "Usage: " << argv[0] << " <expected_aspect_ratio> <netlist_file> <symmetry_constraint_file> <building_block_file> <output_file>" << endl;
        return 1;
    }
    srand(time(0));
    // 從命令行的參數取得 expected_aspect_ratio, netlist, symmetry constraint, building block, output file
    ifstream netlistFile(argv[2], ios::in);
    ifstream symmetryFile(argv[3], ios::in);
    ifstream buildingBlockFile(argv[4], ios::in);
    ofstream outputFile(argv[5],ios::out); 
    vector<string> placement;

    if(!netlistFile || !symmetryFile || !buildingBlockFile || !outputFile) {
        cerr << "File could not be opened" << endl;
        exit(1);
    }   
    parseNetlist(netlistFile);
    parseBuildingBlock(buildingBlockFile);    
    parseSymmetry(symmetryFile);
    BStarTree tree = build_tree_by_net();
    placement = tree.placement;
    placeSymmetry(placement, tree.chip_width, tree.chip_height);
    generateOutput(placement, outputFile);
}   //end main
