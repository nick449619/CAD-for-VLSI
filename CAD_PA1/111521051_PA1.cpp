/*

Author: 卓祐晟
Creation Date: 2024.03.07
Last Modified: 2024.03.17
Description: convert circuit description from .isc file into verilog file

*/



#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sstream>  //分割字串

using namespace std;

// 用一組 struct 紀錄當 type = from 時的 address 及 fansout
// Ex:  8     8fan from     3gat      >sa1  存成 {8, 3}
struct fromTypeLine {
    string fromTypeLine_address;        //address
    string fromTypeLine_fansout;        //fansout
};

//定義 Function
void read_isc(ifstream&, vector<string>&, vector<string>& ,vector<string>&, vector<string>&);  // 讀取 ISC 檔案，並將需輸出的資料儲存下來 
void generate_verilog(ofstream&, const vector<string>&, const vector<string>&, const vector<string>&,const vector<string>&, const string&) ; // 將儲存的資料輸出成 Verilog 
string check_fromtype_fansout(const vector<fromTypeLine>&, const string&); //要輸出時, 檢查輸出的結果是否有因為之前的 From type 需要更換 gat_number
string check_wireout(const vector<string>&, const string&); //要輸出時, 檢查輸出的結果要是 gat 還是 gat_out

int main(int argc, char *argv[]) {
    // 檢查命令行的參數數量, argc為命令數量, argv為字串的陣列
    if ( argc != 3 ){
        cerr << "Input Type : " << argv[0] << "iscfile verilogfile" << endl;
        return 1;
    }

    // 從命令行的參數取得 ISC filename 與 Verilog filename
    string myIscFile = argv[1];
    string myVerilogFile = argv[2];
    ifstream inputfile(myIscFile, ios::in);
    ofstream outputfile(myVerilogFile, ios::out);

    // moduleName 為 ISC filename 去掉後面四個字元, EX: c432.isc -> c432
    string moduleName =  myIscFile.substr(0, myIscFile.length() - 4);
    
    //exit program if fstream couldg not open the file
    if(!inputfile || !outputfile) {
        cerr << "File could not be opened" << endl;
        exit(1);
    }   
    // 從 isc 一行一行讀取, 儲存需記錄的資料, 再將結果輸出成 verilog
    vector<string> inputAddress;                // 當 type = inpt，存取 inputAddress
    vector<string> outputAddress;               // 當 type = logic gate 且 fansout = 0，存取 outputAddress
    vector<string> wireAddress;                 // 當 type = logic gate 且 fansout != 0，存取 wireaddress
    vector<string> gateDescription;             // 當遇到 logic gate 的輸出時，將須輸出成 verilog 的結果先以行為單位存成字串, 並存成 vector, Ex: nand gat10 (gat_out10, gat1, gat3); 為一個字串
   

    // 讀取 ISC 文件並存取需要輸出的資料
    read_isc(inputfile, inputAddress, outputAddress, wireAddress, gateDescription);
    // 生成 verilog code
    generate_verilog(outputfile, inputAddress, outputAddress, wireAddress, gateDescription, moduleName);
    return 0;
}   //end main

// 讀取 ISC 文件並存取需要輸出的資料
void read_isc(ifstream& inputfile, vector<string>& inputAddress, vector<string>& outputAddress, vector<string>& wireAddress, vector<string>& gateDescription) {

    string line;                //ISC 檔案拆成以行為單位
    string word;                   // 把行以空格為單位拆成每一個 word
    vector<string> words;           // 將 word 存成 vector
    string address;             // 不是邏輯閘的輸入時，words[0]   
    string type;                // 不是邏輯閘的輸入時，words[2]
    string fansout;                // 不是邏輯閘的輸入時，words[3]
    vector<fromTypeLine> myfromTypeLineRecord;  // 當 type = from 時，記錄每組的 address 及 fansout, 並存成 vector, Ex:  8     8fan from     3gat      >sa1  存成 {8, 3}的結構
    bool is_logicgate_input_statement = 0;   // 判斷是不是邏輯閘得輸入，如果 type = logic gate ，下一行即是邏輯閘的輸入
    
    // 以行為單為做讀取
    while(getline(inputfile, line)){
        if (line.at(0) == '*') continue;             // * 為 ISCAS85 comement     
        // 切割字串
        istringstream ss(line);
        while(ss >> word){
            words.push_back(word);       
        }
        //切割字串  
        
        if(is_logicgate_input_statement){                    // 判斷是不是邏輯閘的輸出
            /* 如果是, 則準備輸出至 verilog ，將資料存成以行為單位存成字串
               Example :
               19    19gat nand    1   2      >sa1
                15     7                                ->   nand gat19 (gat_out19, gat_out11, gat7);
            */
            string gateDescriptionTemp = type + " gat" + address + " (gat_out" + address; // 上一行的 type 和 address
            for (size_t i = 0; i < words.size(); i++) { 
                words[i] = check_fromtype_fansout(myfromTypeLineRecord, words[i]);       // 檢查之前的 fromTypeRecord，並看是否需更換gat_number (Ex: 15 換成 11)    
                gateDescriptionTemp.append(", " + check_wireout(wireAddress, words[i])); // 檢查之前的 wireAddress, 並看是否 gat 要更換成 gat_out
            }
            
            gateDescriptionTemp.append( ");" );
            gateDescription.push_back(gateDescriptionTemp);     //以行為字串存成 vector
            is_logicgate_input_statement = 0;                    // Reset is_logicgate_input_statement
        }

        else                                            
        {   
            type = words[2];                            
            fansout = words[3];
            address = words[0];
            // 如果不是邏輯閘的輸入, 判斷是哪種 type 
            if(type == "from"){     // 處理 type = from 的類型, Ex:  8     8fan from     3gat      >sa1  存成 {8, 3} 的 struct，並 push 至 myfromTypeLineRecord
                fromTypeLine fromTypeLineTemp;
                fromTypeLineTemp.fromTypeLine_address = address;
                fromTypeLineTemp.fromTypeLine_fansout = fansout.substr(0, fansout.size() - 3); 
                myfromTypeLineRecord.push_back(fromTypeLineTemp);
            }
            else if(type == "inpt") {    // type = inpt 的類型, 將 address push 至 inputAddress
                inputAddress.push_back("gat" + address);
            }
            else{                        // type = logicGate 的類型, 根據 fansout 判斷要將 address push 至 outputAddress 或 wireAddress
                if(fansout == "0"){
                    outputAddress.push_back("gat_out" + address);
                }
                else  {
                    wireAddress.push_back("gat_out" + address);
                }
                is_logicgate_input_statement = 1;   // 下一行為邏輯閘的input    
            }
        }
        words.clear();
    }   //endwhile
}

// 生成 verilog code, 註解以 c17 為範例
void generate_verilog(ofstream& outputfile,const vector<string>& inputAddress,const vector<string>& outputAddress,const vector<string>& wireAddress,const vector<string>& gateDescription, const string& moduleName){
    outputfile << "`timescale 1ns/1ps" << endl;

    // module c17 (gat1, gat2, gat3, gat6, gat7, gat_out22, gat_out23);
    vector<string> module_print ; // inputAddress 和 outputAddress 
    module_print.insert(module_print.end(), inputAddress.begin(), inputAddress.end());
    module_print.insert(module_print.end(), outputAddress.begin(), outputAddress.end());
    outputfile << "module " << moduleName << " (";
    for (size_t i = 0; i < module_print.size(); i++) {
        outputfile << module_print[i] ;
        if (i != module_print.size() - 1)
            outputfile << ", ";
    }
    outputfile << ");" <<endl;

    // input gat1, gat2, gat3, gat6, gat7;
    outputfile << "input " ;
    for (size_t i = 0; i < inputAddress.size(); i++) {
        outputfile << inputAddress[i] ;
        if (i != inputAddress.size() - 1)
            outputfile << ", ";
    }
    outputfile << ";" <<endl;
    
    // output gat_out22, gat_out23;
    outputfile << "output " ;
    for (size_t i = 0; i < outputAddress.size(); i++) {
        outputfile << outputAddress[i] ;
        if (i != outputAddress.size() - 1)
            outputfile << ", ";
    }
    outputfile << ";" <<endl;
    
    // wire gat_out10, gat_out11, gat_out16, gat_out19;
    outputfile << "wire " ;
    for (size_t i = 0; i < wireAddress.size(); i++) {
        outputfile << wireAddress[i] ;
        if (i != wireAddress.size() - 1)
            outputfile << ", ";
    }
    outputfile << ";" <<endl;
    
    /*
    nand gat10 (gat_out10, gat1, gat3);         
    nand gat11 (gat_out11, gat3, gat6);
    nand gat16 (gat_out16, gat2, gat_out11);
    nand gat19 (gat_out19, gat_out11, gat7);
    nand gat22 (gat_out22, gat_out10, gat_out16);
    nand gat23 (gat_out23, gat_out16, gat_out19);
    */
   
    for(size_t i = 0; i < gateDescription.size(); i++){
        outputfile << gateDescription[i] << endl;
    }
    outputfile << "endmodule" ;
}

// 查找函式，查找 logic_gate_input 有沒有在 fromTypeRecord
string check_fromtype_fansout(const vector<fromTypeLine>& data, const string& gateInput) {
    for (const auto item : data) {
        if (item.fromTypeLine_address == gateInput) {
            return item.fromTypeLine_fansout;
        }
    }
    // 如果找不到對應地址的 fromTypeLine_fansout，返回原來查找的字串
    return gateInput;
}

// 查找函式, 查找 logic_gate_input 有沒有在 wireAddress
string check_wireout(const vector<string>& data, const string& gatInput){
    for (const auto item : data) {
        if(item == "gat_out" + gatInput) {
            return "gat_out" + gatInput;
        }
    }
    return "gat" + gatInput;
} 
