#include "chat.hpp"

    
    // Constructors
    //----------------------------------------------------------------------------

    chat::chat()
    {
        vector<string> message_history = {};
        vector<string> message_author = {};
        system_message = "";
        temperature=0.25;
        top_k = 40;
        top_p = 0.9;
    }
    
    chat::chat(string message, string author, string system_message_in)
    {
        vector<string> message_history;
        vector<string> message_author;
        message_history.push_back(message);
        system_message = "";
        temperature=0.25;
        top_k = 40;
        top_p = 0.9;
    }
    
    chat::chat(vector<string> histories, vector<string> authors, string system_message_in, float temperature_in, int top_k_in, float top_p_in)
    {
        message_history = histories;
        message_author = authors;
        system_message = system_message_in;
        temperature = temperature_in;
        top_k = top_k_in;
        top_p = top_p_in;
    }


    // Message handlers
    //----------------------------------------------------------------------------

    /*
     * @param message - the message to add to the message_history vector
     * @param author - the author of the message to add to the message_author vector
     */
    void chat::add_message(const string& message, const string& author)
    {
        message_history.push_back(message);
        message_author.push_back(author);
    }
    
    /*
     * @param index - the index of the message to return
     * @return the message at the given index in the message_history and message_author vectors
     * Format is ["message", "author"]
     */
    vector<string> chat::get_message(int index)
    {
        vector<string> last_message;
        last_message.push_back(message_history[index]);
        last_message.push_back(message_author[index]);
        return last_message;
    }
    
    /*
     * @return the message at the last index in the message_history and message_author vectors
     * Format is ["message", "author"]
     */
    vector<string> chat::get_last_message()
    {
        vector<string> last_message;
        last_message.push_back(message_history.back());
        last_message.push_back(message_author.back());
        return last_message;
    }
    

    // System context handlers
    //----------------------------------------------------------------------------
    void chat::write_system_message(const string& message)
    {
        system_message = message;
    }
    
    string chat::retreive_system_message()
    {
        return system_message;
    }


    // Settings handlers
    //----------------------------------------------------------------------------
    void chat::set_temperature(float temperature_in)
    {
        temperature = temperature_in;
    }
    
    void chat::set_top_k(int top_k_in)
    {
        top_k = top_k_in;
    }
    
    void chat::set_top_p(float top_p_in)
    {
        top_p = top_p_in;
    }

    float chat::get_temperature() const
    {
        return temperature;
    }

    int chat::get_top_k() const
    {
        return top_k;
    }

    float chat::get_top_p() const
    {
        return top_p;
    }
    
    // History handlers
    //----------------------------------------------------------------------------

    /*
     * This function returns a copy of the message history stored in the chat object. 
     * The message history is stored as two parallel vectors, one containing the messages and
     * one containing the authors of those messages.
     * The function returns a vector of vectors, where each inner vector contains a message and its author.
     */
    vector<vector<string>> chat::get_history() const
    {
        vector<vector<string>> history;
        
        for (int i = 0; i < message_history.size(); i++)
        {
            history.push_back({message_history[i], message_author[i]});
        }

        return history;
    }
    
    int chat::get_history_size() const
    {
        return message_history.size();
    }
    
