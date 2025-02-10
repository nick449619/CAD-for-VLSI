#include <iostream>

struct yContourNode {
    double x0;
    double xf;
    double y;
    yContourNode* prev;
    yContourNode* next;
};

using namespace std;
double my_yContour(yContourNode* head, double place_x, double blockwidth);
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
        cout << "Error: Cannot find startOverlap node." << endl;
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
        cout << "Error: Cannot find endOverlap node." << endl;
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


int main() {
    yContourNode* head = new yContourNode; // 分配 head 的記憶體
    head->x0 = 0;
    head->xf = 1000000000;
    head->y = 0;
    head->prev = nullptr; // 設置 head 的前後指標為空
    head->next = nullptr;
    // 假設您已經有了一個填充了高度信息的 yContourNode 鏈表
    // 在這裡插入一些節點作為示例
    double place_x = 0;
    double blockwidth = 100.0; // 假設方塊寬度為1.0
    insertNode(head, 0.0, 10.0, 2.0);
    cout << "done insert 1" << endl;

    insertNode(head, 10.0, 20, 3.0);
    cout << "done insert 2" <<endl;
    
    insertNode(head, 30, 40, 3.0);
    cout << "done insert 2" <<endl;

    insertNode(head, 30, 50, 4);
    cout << "done insert 3" << endl;

    double y = my_yContour(head, 80, 5);
    cout << y << endl;
    return 0;
}

