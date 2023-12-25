#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <cstdlib>
#include <unistd.h>
#include <deque>
#include <ctime>


struct Customer;

const int COUNT_SHOPS_MAX = 2;
const int COUNT_BUYERS_MAX = 7;
const int COUNT_LIST_PRODUCT_SIZE = 6;

pthread_mutex_t output;
pthread_mutex_t orderWriteInQueue;
pthread_mutex_t orderService;
pthread_mutex_t waitQueue;

std::deque<Customer*>* queueShops = nullptr;


struct Customer {
    int* shoppingList;
    int numberProduct;
    int countProduct;
    int BYUER_ID;

    Customer(int* shoppingList, int countProduct, int BYUER_ID) : shoppingList(shoppingList), countProduct(countProduct), BYUER_ID(BYUER_ID) {}
};

class OutputInfo {
    public:
        void OutputBuyInfo(int BYUER_ID, int SHOP_ID, int numberProduct) {
            pthread_mutex_lock(&output);
            printf("Покупатель:%d купил в магазине:%d товар под %d номером в списке\n", BYUER_ID, SHOP_ID, numberProduct);
            pthread_mutex_unlock(&output);
        }
        void OutputWaitInfo(int BYUER_ID, int SHOP_ID) {
            pthread_mutex_lock(&output);
            printf("Покупатеь:%d ждет в очереди в магазин:%d\n", BYUER_ID, SHOP_ID);
            pthread_mutex_unlock(&output);
        }
        void OutputServiceInfo(int BYUER_ID, int SHOP_ID) {
            pthread_mutex_lock(&output);
            printf("Покупатель:%d обслуживается магазином:%d\n", BYUER_ID, SHOP_ID);
            pthread_mutex_unlock(&output);
        }
        void OutputAllBuyInfo(int BYUER_ID) {
            pthread_mutex_lock(&output);
            printf("Покупатель:%d все купил по списку!\n", BYUER_ID);
            pthread_mutex_unlock(&output);
        }

        void OutputListProduct(int BYUER_ID, int* shoppingList, int countProduct) {
            pthread_mutex_lock(&output);
            printf("Покупателю %d нужно: ", BYUER_ID);
            for(int i = 0; i < countProduct; ++i) {
                std::cout << shoppingList[i] << " ";
            }
            printf("\n");
            pthread_mutex_unlock(&output);
        }

};

OutputInfo outputInfo;

void* Shop(void *param) {
    int SHOP_ID = *((int*)param);
    
    while(1) {
        if (!queueShops[SHOP_ID].empty()) {
            // Service
            pthread_mutex_lock(&orderWriteInQueue);
            Customer* customer = queueShops[SHOP_ID].front();
            queueShops[SHOP_ID].pop_front();
            outputInfo.OutputServiceInfo(customer->BYUER_ID, SHOP_ID); 
            while(customer->numberProduct < customer->countProduct && customer->shoppingList[customer->numberProduct] == SHOP_ID) {
                //outputInfo.OutputBuyInfo(customer->BYUER_ID, SHOP_ID, customer->numberProduct + 1);
                customer->numberProduct++;
            }
            sleep(6);
            pthread_mutex_unlock(&orderWriteInQueue);
        }
    }

    return nullptr;
}

void* Buyer(void *param) {

    Customer* customer = (Customer*)param;
    int index = 0;
    while (index < customer->countProduct) {
        // Block write in line output
        pthread_mutex_lock(&waitQueue);
        outputInfo.OutputWaitInfo(customer->BYUER_ID, customer->shoppingList[index]);
        pthread_mutex_unlock(&waitQueue);
        // Write in line
        pthread_mutex_lock(&orderWriteInQueue);
        queueShops[customer->shoppingList[index]].push_back(customer);
        while(index + 1 < customer->countProduct && customer->shoppingList[index + 1] == customer->shoppingList[index]) {
            index++;
        }
        
        index++;

        pthread_mutex_unlock(&orderWriteInQueue);
        sleep(2);
    }

    while(customer->numberProduct < customer->countProduct);
    outputInfo.OutputAllBuyInfo(customer->BYUER_ID);
    return nullptr;
}



int main() {

    // Inicialization random generator
    srand(time(NULL));

    // Inicialization mutex
    pthread_mutex_init(&orderWriteInQueue, nullptr);
    pthread_mutex_init(&orderService, nullptr);
    pthread_mutex_init(&waitQueue, nullptr);
    pthread_mutex_init(&output, nullptr);
    
    // Count shops and buyers
    int countBuyers = (rand() % COUNT_BUYERS_MAX) + 1;
    int countShops = 2; //(rand() % COUNT_SHOPS_MAX) + 1;

    queueShops = new std::deque<Customer*>[countShops];
    int* shopsId = new int[countShops];

    // Arrays shops and buyers
    pthread_t buyers[countBuyers];
    pthread_t shops[countShops];


    // Start shops
    for(int i = 0; i < countShops; ++i) {
        shopsId[i] = i;
        pthread_create(&shops[i], NULL, Shop, &shopsId[i]);
    }

    // Wait start shops
    sleep(5);

    // Create data for customer
    int** buyersShoppingLists = new int*[countBuyers];
    int* buyersShoppingListsSize = new int[countBuyers];
    for(int i = 0; i < countBuyers; ++i) {
        int countProduct = (rand() % COUNT_LIST_PRODUCT_SIZE) + 1;
        buyersShoppingLists[i] = new int[countProduct];
        buyersShoppingListsSize[i] = countProduct;
        for(int p = 0; p < countProduct; ++p) {
            buyersShoppingLists[i][p] = rand() % countShops;
        }
        outputInfo.OutputListProduct(i + 1, buyersShoppingLists[i], countProduct);
    }

    // Start buyers
    for(int i = 0; i < countBuyers; ++i) {
        pthread_create(&buyers[i], NULL, Buyer, (void*)(new Customer(buyersShoppingLists[i], buyersShoppingListsSize[i], i + 1)));
        // Waiting to enter the store
        sleep(1);
    }

    // Wait exit shopping
    for(int k = 0; k < countBuyers; ++k) {
        pthread_join(buyers[k], NULL);
    }

    printf("Все обслужаны!");

    // Free memory
    delete[] shopsId;
    for(int i = 0; i < countBuyers; ++i) {
        delete[] buyersShoppingLists[i];
    }
    delete[] buyersShoppingLists;
    delete[] buyersShoppingListsSize;

    pthread_mutex_destroy(&orderWriteInQueue);
    pthread_mutex_destroy(&waitQueue);
    pthread_mutex_destroy(&orderService);
    pthread_mutex_destroy(&output);

    return 0;
}