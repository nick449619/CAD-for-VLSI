/*
Author: 卓祐晟
Creation Date: 2024.4.14
Last Modified: 2024.4.22
Description: 
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <random>
#include <sstream>  //分割字串
#include <unordered_map> // 把 myNetarray 轉成 cellarray
#include <ctime>
#include <chrono> // 計算程式執行時間
#include <algorithm> // find, random_shuffle


using namespace std;
struct partitionCase{
    vector<string> groupA;
    vector<string> groupB;
};

// global variable
unordered_map<string, vector<string>> cellNetRelation;
vector<vector<string>> myNetArray;

// function
void read_file(ifstream&);
void generate_cellArray(const vector<vector<string>>& );
partitionCase random_partition(const unordered_map<string, vector<string>>&); // initial partition 
partitionCase simulated_annealing(const partitionCase&);
double move_to_neighbor_possibility(int, int, double);  // SA算法用到的function
partitionCase  move_to_neighbor(partitionCase&);    // SA算法用到的function
int cal_cutsize(const partitionCase&);    // SA算法用到的function
vector<string> findNetForCell(const unordered_map<string, vector<string>>& , const string& );  //為了計算cutsize, 查找cell對應的net
void generate_outputfile(ofstream&, const partitionCase& , const int );


// main
int main(int argc, char *argv[]) {
    // 檢查命令行的參數數量, argc為命令數量, argv為字串的陣列
    if ( argc != 3 ){
        return 1;
    }

    // 在程式碼開始之前取得開始時間點
    auto start_time = chrono::steady_clock::now();

    string input = argv[1];
    string output = argv[2];
    ifstream inputfile(input, ios::in);
    ofstream outputfile(output, ios::out);
    
    srand(time(0));
    partitionCase mypartition;
    int best_cutsize;
    read_file(inputfile);
    mypartition = random_partition(cellNetRelation);    //first partition
    int count = 1;
    while(count <= 5){
        mypartition = simulated_annealing(mypartition);
        best_cutsize = cal_cutsize(mypartition);
        generate_outputfile(outputfile, mypartition, best_cutsize);
        count++;
    }
    
    // 在程式碼結束後取得結束時間點
    auto end_time = chrono::steady_clock::now();
    // 計算執行時間
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);
    cout << "Excution Time: " << duration.count() << " s" << endl;
    return 0;
}
//end main

int cal_cutsize(const partitionCase& partition) {
    vector<string> netlistA;
    vector<string> netlist_temp;
    int cutsize = 0;
    // 先生成 groupA 的有出現的 net
    for(const string cell : partition.groupA){
        netlist_temp = findNetForCell(cellNetRelation, cell);
        for(const string net : netlist_temp){
            auto it = find(netlistA.begin(), netlistA.end(), net);
            if(it == netlistA.end()){               //如果沒找到，則push進來
                netlistA.push_back(net);            //不把重複的net push進來
            }
        }
    }
    // 找看看有沒有和 partitonA 重複的 net
    for(const string cell : partition.groupB){
        netlist_temp = findNetForCell(cellNetRelation, cell);
        for(const string net : netlist_temp){
            auto it = find(netlistA.begin(), netlistA.end(), net);  //只要找到一個立刻返回 True
            if(it!= netlistA.end())  {              
                netlistA.erase(it);
                cutsize++;
            }
        }
    }
    return cutsize;
}

// 比較和前一次的結果
double move_to_neighbor_possibility(int old_cost, int new_cost, double temperature) {
    if (new_cost < old_cost) {      //新的比舊的好
        return 1;
    } else {
        return exp((old_cost - new_cost) / temperature);
    }
}

// 定義 Simulated Annealing 算法
partitionCase simulated_annealing(const partitionCase& initial_partition) {
    partitionCase current_partition = initial_partition;
    partitionCase previous_partition = initial_partition;
    partitionCase best_partition = initial_partition;
    int current_cost = cal_cutsize(current_partition);
    double temperature = 10;  
    double cooling_rate = 0.998;
    double freezingPoint = 0.001;
    int best_cost = current_cost;

    while(temperature >= freezingPoint) {
        
        partitionCase new_partition;
        new_partition = move_to_neighbor(current_partition);
        int new_cost = cal_cutsize(new_partition);

        // 查看換到neighbor的機率
        if (rand() / static_cast<double>(RAND_MAX) <= move_to_neighbor_possibility(current_cost, new_cost, temperature)) {  
            current_partition = new_partition;
            current_cost = new_cost;
        }
        else{
            current_partition = previous_partition;
        }
        
        if (current_cost < best_cost){
            best_cost = current_cost;
            best_partition = current_partition;
        }

        // 降低溫度
        previous_partition = current_partition;
        temperature *= cooling_rate;
    }

    return best_partition;
}

void read_file(ifstream& inputfile) {
    string line;
    while(getline(inputfile, line)) {
        istringstream ss(line);
        string word;
        vector<string> row; // Ex: row[0] = n1, row[1] = c2
        while(ss >> word) {
            if (word == "{" || word == "}" || word == "NET") continue; // 忽略大括號
            row.push_back(word);
        }
        myNetArray.push_back(row); // 將暫存的向量添加到 myNetArray 中
    }
    generate_cellArray(myNetArray);
}

void generate_cellArray(const vector<vector<string>>& NetArray) {
    for (const auto& row : NetArray) {
        string net = row[0]; // 每行第一個元素為 net
        for (size_t i = 1; i < row.size(); ++i) {
            string cell = row[i]; // 每行其餘元素為 cell
            cellNetRelation[cell].push_back(net); // 將 cell 加入對應的 net 中
        }
    }
}

vector<string> findNetForCell(const unordered_map<string, vector<string>>& cellNetRelation, const string& cell) {
    // 查找 cell 在 cellNetRelation 中的值
    auto it = cellNetRelation.find(cell);
    if (it != cellNetRelation.end()) {
        return it->second;
    } else {
        return {};
    }
}

partitionCase random_partition(const unordered_map<string, vector<string>>& cellNetRelation) {
    int max_difference = cellNetRelation.size() / 5;
    partitionCase initial;
    for (const auto& entry : cellNetRelation) {
        int x = rand () % 2;
        if ((x == 1 && initial.groupA.size() - initial.groupB.size() <= max_difference) or (initial.groupA.size() <= initial.groupB.size())) {
            initial.groupA.push_back(entry.first);
        }
        else {
            initial.groupB.push_back(entry.first);
        }
    }
    return initial;
}

partitionCase move_to_neighbor(partitionCase& current) {
    double max_difference = (current.groupA.size() + current.groupB.size()) / 5.0;
    int difference = 0;
    vector<string> source, destination;
    int num_to_select;
    partitionCase next;
    // 如果差距超過閾值，則重新隨機選擇元素，直到差距不超過閾值
    do
    {   
        int randselection = rand() % 2; // 隨機選擇要從A移到B,或是B移到A
        if (randselection == 1)
        {
            source = current.groupA;
            destination = current.groupB;
        }
        else{
            source = current.groupB;
            destination = current.groupA;           
        }
  
        num_to_select = 2;       // 選擇隨機數量的元素, 目前是設定一次隨機移 1~2  rand()%2+1

        difference = (source.size() - num_to_select) - (destination.size() + num_to_select) ;

    } while(abs(difference) > max_difference); //如果大於要重選, 小於等於才繼續做下去
    
    
    random_shuffle(source.begin(), source.end());
    // 從 source 向 destination 移動元素
    for (int i = 0; i < num_to_select; ++i) {
        destination.push_back(source.back());
        source.pop_back();
    }
    current.groupA = source;
    current.groupB = destination;

    return current;
}

void generate_outputfile(ofstream& outputfile, const partitionCase& best_partition, const int best_cutsize){
    // 清空輸出文件
    outputfile.clear();
    outputfile.seekp(0, ios::beg);

    outputfile << "cut_size " << best_cutsize << endl; 
    outputfile << "A" <<endl;
    for(const auto item : best_partition.groupA){
        outputfile << item << endl;
    }
    outputfile << "B"  <<endl;
    for(const auto item : best_partition.groupB){
        outputfile << item << endl;
    }
}

