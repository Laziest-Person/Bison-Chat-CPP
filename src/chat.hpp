#ifndef CHAT_H
#define CHAT_H

#include <string>
#include <vector>

using namespace std;

class chat {
    
public:
    
    // Constructors
    chat();
    chat(string message, string author, string system_message);
    chat(vector<string> history, vector<string> author, string system_message, float temperature_in, int top_k_in, float top_p_in);
    // Message handlers
    void add_message(const string& message, const string& author);
    vector<string> get_message(int index);
    vector<string> get_last_message();
    // System context handlers
    void write_system_message(const string& message);
    string retreive_system_message();
    // Full-history handlers
    vector<vector<string>> get_history() const;
    void print_history() const;
    int get_history_size() const;
    //Settings handlers
    void set_temperature(float temperature);
    void set_top_k(int top_k);
    void set_top_p(float top_p);
    float get_temperature() const;
    int get_top_k() const;
    float get_top_p() const;

private:
    vector<string> message_history;
    vector<string> message_author;
    string system_message;
    float temperature;
    int top_k;
    float top_p;
    
};


#endif