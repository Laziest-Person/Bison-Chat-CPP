#include <gtk/gtk.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <mutex>
#include <regex>
#include <string>
#include <iostream>
#include <fstream>
#include "chat.hpp"

using json = nlohmann::json;
using namespace std;



/*
 * This section is for utility variables and related functions only
 * ------------------------------------------------------------------------------------------*/

string message_completion_url = "https://generativelanguage.googleapis.com/v1beta2/models/chat-bison-001:generateMessage?key=";
string system_prompt;
string api_key;
string user;
string interpreter_name;
chat chat_history;

struct UIElements {
    GtkWidget *message_vbox;
    GtkWidget *window_hbox;
    GtkWidget *scrolled_window;
};

/*
 * This function is the callback function for the libcurl write function.
 * It is responsible for writing the response data to a string.
 *
 * @param ptr The pointer to the response data
 * @param size The size of the response data
 * @param nmemb The number of members in the response data
 * @param data The pointer to the string to write the response data to
 *
 * @return The size of the response data
 *
 * @throws None.
 */
size_t libcurl_write_callback(char *ptr, size_t size, size_t nmemb, std::string *data)
{
    data->append(ptr, size * nmemb);
    return size * nmemb;
}

/*
 * This function reads a file to a string.
 * Then, returns that string
 *
 * @param filename The name of the file to read
 * @return The contents of the file as a string
 *
 * @throws None.
 */
std::string read_file(const std::string &filename)
{
    // Open the file
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filename << "\n";
        return "";
    }

    // Read the file contents into a string
    std::string contents;
    file.seekg(0, std::ios::end);
    contents.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&contents[0], contents.size());

    // Close the file
    file.close();

    return contents;
}

/*
 * This function is responsible for writing a new text box to the message vbox.
 * This function is the version used in main thread calls,
 * the other-thread-end callback is below this one.
 *
 * @param message_vbox The GtkWidget pointer to the message vbox
 * @param message The message to be written to the text box
 *
 * @throws None.
 */
void write_message_text_to_vbox(GtkWidget *message_vbox, GtkWidget *scrolled_window, string message)
{

    // Create a new text view widget
    GtkWidget *message_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(message_view), FALSE);

    // Get the chat buffer
    GtkTextBuffer *chat_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));

    // Insert the text into the chat buffer
    gtk_text_buffer_insert_at_cursor(chat_buffer, message.c_str(), -1);

    // Pack the text view widget into the message vbox
    gtk_box_pack_start(GTK_BOX(message_vbox), message_view, FALSE, FALSE, 0);

    // Run the GTK main loop until all events are processed
    while (gtk_events_pending()) {
    gtk_main_iteration();
    }

    // Get the vertical adjustment of the scrolled window
    GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scrolled_window));

    // Set the value of the adjustment to its maximum value
    gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));

    // Run the GTK main loop until all events are processed
    while (gtk_events_pending()) {
        gtk_main_iteration();
    }

    // Show all widgets
    gtk_widget_show_all(scrolled_window);

}

static void run_code(GtkWidget *widget, UIElements *elements)
{

}

static void remove_prompt_do_not_run_code(GtkWidget *widget, UIElements *elements)
{
    // Remove the prompt vbox from the window hbox

}

/* ------------------------------------------------------------------------------------------
 * End of utility variables and related functions only section
 */



/*
 * This section is for second-thread (receive-message thread) variables and related functions only
 * ------------------------------------------------------------------------------------------*/

// This struct is used to pass data to and from the thread function.
struct ThreadData
{
    GtkWidget *window_hbox;
    GtkWidget *message_vbox;
    GtkWidget *scrolled_window;
    GAsyncQueue *queue;
    GMainContext *context;
    mutex *mtx;
    string completion_url;
    chat *chat_msgs;
    string interpreter_name_str;
    bool code_to_run_exists;
    string code_to_run;
};

/*
 * This function is responsible for writing a new text box to the message vbox.
 * This function is the version used in other-thread-end callbacks,
 * it pops the message from the queue and then makes a TextBox and appends the
 * text box to the message_vbox which has padding between them. This is why messages
 * look like they do in a chat app.
 *
 * @param message_vbox The GtkWidget pointer to the message vbox
 * @param message The message to be written to the text box
 *
 * @throws None.
 */
void add_message_to_message_vbox(ThreadData *data)
{

    data->mtx->lock();
    GtkWidget *message_vbox = (GtkWidget *)data->message_vbox;
    gchar *message = (gchar *)g_async_queue_try_pop(data->queue);

    // Create a new text view widget
    GtkWidget *message_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(message_view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(message_view), FALSE);

    // Get the chat buffer
    GtkTextBuffer *chat_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(message_view));

    // Insert the text into the chat buffer
    gtk_text_buffer_insert_at_cursor(chat_buffer, message, -1);
    gtk_box_pack_start(GTK_BOX(data->message_vbox), message_view, FALSE, FALSE, 0);

    // Run the GTK main loop until all events are processed
    while (gtk_events_pending()) {
    gtk_main_iteration();
    }

    // Get the vertical adjustment of the scrolled window
    GtkAdjustment *adjustment = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(data->scrolled_window));

    // Set the value of the adjustment to its maximum value
    gtk_adjustment_set_value(adjustment, gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));

    // Run the GTK main loop until all events are processed
    while (gtk_events_pending()) {
    gtk_main_iteration();
    }

    // Show all widgets
    gtk_widget_show_all(data->scrolled_window);


    // If there's code, add a prompt in the right side of the window to ask about running it.
    //-------------------------------------------------------------------------------------------
    /*if ( data->code_to_run_exists )
    {
    
        // Create the prompt label
        GtkWidget *prompt_label = gtk_label_new("Do you want to run the code?");
        gtk_label_set_xalign(GTK_LABEL(prompt_label), 1);
    
        // Create a new text view widget for code, code_text_view
        GtkWidget *code_text_view = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(code_text_view), FALSE);
        gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(code_text_view), FALSE);

        //Insert code into code_text_view
        // Get the code_buffer
        GtkTextBuffer *code_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(code_text_view));
        // Insert the text into the code_buffer
        gtk_text_buffer_insert_at_cursor(code_buffer, data->code_to_run.c_str(), -1);

        // Insert the text view into a scrollable window
        GtkWidget *code_text_scrolled_window = gtk_scrolled_window_new(NULL, NULL);
        gtk_container_add(GTK_CONTAINER(code_text_scrolled_window), code_text_view);

        // Create the yes button
        GtkWidget *yes_button = gtk_button_new_with_label("Yes");
        g_signal_connect(G_OBJECT(yes_button), "clicked", G_CALLBACK(run_code), data);

        // Create the no button
        GtkWidget *no_button = gtk_button_new_with_label("No");
        g_signal_connect(G_OBJECT(no_button), "clicked", G_CALLBACK(remove_prompt_do_not_run_code), data);

        //Create yes/no buttons' hbox
        GtkWidget *yes_no_hbox = gtk_hbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(yes_no_hbox), yes_button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(yes_no_hbox), no_button, FALSE, FALSE, 0);

        //Create and populate the prompt vbox
        GtkWidget *prompt_vbox = gtk_vbox_new(FALSE, 0);
        gtk_box_pack_start(GTK_BOX(prompt_vbox), prompt_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(prompt_vbox), code_text_scrolled_window, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(prompt_vbox), yes_no_hbox, FALSE, FALSE, 0);

        //Add the prompt vbox to the window hbox
        gtk_box_pack_end(GTK_BOX(data->window_hbox), prompt_vbox, FALSE, FALSE, 0);
    }*/

    data->mtx->unlock();

    g_free(message);

    return;
}

/* Passes the gpointer converting to ThreadData struct
 * from the other thread on to the add_message_to_message_vbox function.
 * (Needed for GTK Main thread and other thread conflict issues)
 */
gboolean add_message_to_message_vbox_wrapper(gpointer user_data)
{
    ThreadData *data = (ThreadData *)user_data;
    add_message_to_message_vbox(data);
    return FALSE;
}

/*
 * Find out whether Python or Bash comes first in the message
 *
 * @param message_received The message to search for the first language
 * @return The first language, "python", "bash", or "none"
 *  
 * @throws None.
 */
string find_first_language(string message_received)
{
    string python_string1 = "```python";
    string python_string2 = "```Python";
    string bash_string1 = "```bash";
    string bash_string2 = "```Bash";

    size_t python_string1_pos = message_received.find(python_string1);
    size_t python_string2_pos = message_received.find(python_string2);
    size_t bash_string1_pos = message_received.find(bash_string1);
    size_t bash_string2_pos = message_received.find(bash_string2);

    if (python_string1_pos == string::npos && python_string2_pos == string::npos && bash_string1_pos == string::npos && bash_string2_pos == string::npos)
    {
        //If neither language is present
        return "none";
    }
    else if ((bash_string1_pos == string::npos && bash_string2_pos == string::npos) && (python_string1_pos != string::npos || python_string2_pos != string::npos))
    {
        //If Bash isn't present but Python is present
        return "python";
    }
    else if ((python_string1_pos == string::npos && python_string2_pos == string::npos) && (bash_string1_pos != string::npos || bash_string2_pos != string::npos))
    {
        //If only Bash is present
        return "bash";
    }
    else
    {
        //If both languages are present
        size_t python_pos = min(python_string1_pos, python_string2_pos);
        size_t bash_pos = min(bash_string1_pos, bash_string2_pos);
        if (python_pos < bash_pos)
        {
            //If Python comes first
            return "python";
        }
        else
        {
            //If Bash comes first
            return "bash";
        }   
    }


}

/*
 * This function is responsible for finding the snippets of python markdown syntax in a message
 *
 * @param message The message to search for the markdown syntax
 *
 * @return A vector of snippets from markdown syntax
 */
vector<smatch> find_markdown_python(string message)
{

    vector<smatch> python_snippets;
    string sub_string;
    regex python("```python\n.*\n```");
    smatch match;

    sub_string = message;
    while (regex_search(sub_string, match, python)) 
    {
        cout << "Match found:\n" << match.str() << endl;
        cout << "Match position:\n" << match.position() << endl;
        cout << "Match length:\n" << match.length() << endl;
        python_snippets.push_back(match);
        int start_index = match.position();
        int end_index = match.position() + match.length();
        sub_string = sub_string.substr(end_index, (sub_string.length() - end_index) );
    }

    return python_snippets;

}

/*
 * This function is responsible for finding the snippets of bash markdown syntax in a message
 *
 * @param message The message to search for the markdown syntax
 *
 * @return A vector of snippets from markdown syntax
 */
vector<smatch> find_markdown_bash(string message)
{
    vector<smatch> bash_snippets;
    string sub_string;
    regex bash("```bash\n.*\n```");
    smatch match;

    sub_string = message;
    while (regex_search(sub_string, match, bash)) 
    {
        cout << "Match found:\n" << match.str() << endl;
        cout << "Match position:\n" << match.position() << endl;
        cout << "Match length:\n" << match.length() << endl;
        bash_snippets.push_back(match);
        int start_index = match.position();
        int end_index = match.position() + match.length();
        sub_string = sub_string.substr(end_index, (sub_string.length() - end_index) );
    }

    return bash_snippets;

}

/*
 * This function is responsible for running the message completion query.
 *
 * @param thread_data The ThreadData pointer to the thread data
 *
 * @throws None.
 */
void receive_thread_function(ThreadData *thread_data)
{

    ThreadData *data = (ThreadData *)thread_data;
    data->mtx->lock();
    string system_message = data->chat_msgs->retreive_system_message();
    data->mtx->unlock();
    string message_received;
    json messages_array = json::array();
    json examples_array = json::array();

    // Populate the messages array from the chat history
    data->mtx->lock();
    for (int i = 0; i < data->chat_msgs->get_history_size(); i++)
    {
        vector<string> message = data->chat_msgs->get_message(i);
        messages_array.push_back({{"content", message[0]},{"author", message[1]}});
    }
    data->mtx->unlock();

    // Template for the POST JSON data
    json post_data = {
        {"prompt", {{"context", system_message}, {"examples", examples_array}, {"messages", messages_array}}},
        {"temperature", data->chat_msgs->get_temperature()},
        {"top_k", data->chat_msgs->get_top_k()},
        {"top_p", data->chat_msgs->get_top_p()},
        {"candidateCount", 1}};

    // Convert the JSON data to a string
    string post_json_string = post_data.dump();

    //-----------------------------------------------------------------------------
    //libcurl POST request

    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curl = curl_easy_init();

    if (curl)
    {
        // Set the URL to send the POST request to
        curl_easy_setopt(curl, CURLOPT_URL, data->completion_url.c_str());

        // Set the request method to POST
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        // Set the request body to the JSON data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_json_string.c_str());

        // Set the content type header to indicate JSON data
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        // Set the write callback function and the response data
        string response_data;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, libcurl_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        // Send the request and receive the response
        CURLcode res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
            message_received = "<System:> Sorry, I'm having trouble connecting to the server.";
        }
        else
        {
            // Print the response data to the console (Also annoying me, commented out)
            // std::cout << "Response data: " << response_data << std::endl;
            message_received = json::parse(response_data)["candidates"][0]["content"].get<std::string>();
        }

        // Clean up
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }

    // Clean up libcurl
    curl_global_cleanup();

    //-----------------------------------------------------------------------------
    //Post-libcurl POST request, string post-processing

    // Find which language comes first, or none
    string first_language_name = find_first_language(message_received);

    // Find the instances of code markdown in the message received
    vector<smatch> markdown = {};

    if (first_language_name == "python")
    {
        markdown = find_markdown_python(message_received);
    }
    else if (first_language_name == "bash")
    {
        markdown = find_markdown_bash(message_received);
    }

    // Add the message to the terminal (pseudo-debugging thing that's annoying so I commented it out)
    //cout << "Message received:\n" << message_received << "\n";

    // Cut out the portion after the markdown, if it's present
    string message_post_stripping = "";
    data->code_to_run = "";
    if (!markdown.empty())
    {
        data->mtx->lock();
        data->code_to_run_exists = true;
        data->mtx->unlock();
        message_post_stripping = message_received.substr(0, markdown[0].position() + markdown[0].length());
        //Strip out and isolate the code for code_to_run
        if (first_language_name == "python") data->code_to_run = markdown[0].str().substr( ( markdown[0].str().find_first_of("```") + 10 ) , ( ( markdown[0].str().find_last_of("```") - 2 ) - ( markdown[0].str().find_first_of("```") + 10 ) ) );
        else if (first_language_name == "bash") data->code_to_run = markdown[0].str().substr( ( markdown[0].str().find_first_of("```") + 8 ) , ( ( markdown[0].str().find_last_of("```") - 2 ) - ( markdown[0].str().find_first_of("```") + 8 ) ) );
        cout << "Code to run:\n" << data->code_to_run << endl;
    } else
    {
        message_post_stripping = message_received;
    }

    //-----------------------------------------------------------------------------
    //Post-string-post-processing

    // Add the message to the message_to_output string
    string message_to_output = ("\nBison: " + message_post_stripping + "\n");
    // Add the message to the chat history
    data->mtx->lock();
    data->chat_msgs->add_message(message_post_stripping, data->interpreter_name_str);

    // Add the message to the terminal (pseudo-debugging thing that's annoying so I commented it out)
    //cout << "Message stripped:\n" << message_to_output << "\n";

    // Push the message to the queue
    g_async_queue_push(data->queue, g_strdup(message_to_output.c_str()));

    // Invoke a function in the main thread to add the new widget to the message vbox
    g_main_context_invoke(data->context, add_message_to_message_vbox_wrapper, data);
    data->mtx->unlock();

}

/* ------------------------------------------------------------------------------------------
 * End of second-thread (receive thread) variables and related functions only section
 */



/*
 * This section is for code-execution thread variables and related functions only
 * ------------------------------------------------------------------------------------------*/

    struct ExecutionThreadData
    {
        
    };

/* ------------------------------------------------------------------------------------------
 * End of code-execution-thread variables and related functions only section
 */



/*
 * This section is for main-thread variables and related functions only
 * ------------------------------------------------------------------------------------------*/

/**
 * This function is the main loop interface for the interpreter.
 * It is called repeatedly to execute the interpreter's main loop.
 *
 * @param user_data A pointer to user data that was passed to the main loop.
 * @return A gboolean indicating whether the main loop should continue running (TRUE) or stop (FALSE).
 */
gboolean main_loop_interface(gpointer user_data)
{



    return TRUE;
}

/**
 * This function is responsible for sending the message to the chat history and
 * running the message completion query.
 *
 * @param widget The GtkWidget pointer to the submit button
 * @param message_vbox The GtkWidget pointer to the message vbox
 *
 */
static void send_message(GtkWidget *widget, UIElements *elements)
{

    // Get the entry field
    GtkEntry *entry = GTK_ENTRY(g_object_get_data(G_OBJECT(widget), "attached_entry_field"));

    // Get the text from the entry field
    const string message = gtk_entry_get_text(entry);

    // Add the message to the chat history
    chat_history.add_message(message, user);

    // Insert the text part of the message into the chat vbox
    write_message_text_to_vbox(elements->message_vbox, elements->scrolled_window, ("\nYou: " + message + "\n").c_str());

    // Thread stuff
    ThreadData *thread_data = new ThreadData;
    thread_data->mtx = new mutex;
    thread_data->message_vbox = elements->message_vbox;
    thread_data->scrolled_window = elements->scrolled_window;
    thread_data->window_hbox = elements->window_hbox;
    thread_data->queue = g_async_queue_new();
    thread_data->context = g_main_context_default();
    thread_data->completion_url = message_completion_url;
    thread_data->code_to_run_exists = false;
    thread_data->code_to_run = "";
    thread_data->chat_msgs = &chat_history;
    thread_data->interpreter_name_str = interpreter_name;

    // Init a thread to run the message query
    std::thread my_thread(receive_thread_function, thread_data);
    // Detach the thread
    my_thread.detach();

    // Clear the entry field
    gtk_entry_set_text(entry, "");
}

/* ------------------------------------------------------------------------------------------
 * End of main-thread variables and related functions only section
 */



/*
 * This section is for main function only
 * ------------------------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{

    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create a new chat history object
    chat_history = chat();
    
    // Get the system prompt
    system_prompt = read_file("./res/system_message.txt");
    chat_history.write_system_message(system_prompt);
    
    // Get the config data (mostly API keys)
    string raw_config_json_text = read_file("./res/config.json");
    // Parse the config
    json config_json = json::parse(raw_config_json_text);
    // Get the API key
    api_key = config_json["MakerSuiteKey"];
    // Get the user
    user = config_json["UserName"];
    // Get the interpreter name
    interpreter_name = config_json["InterpreterName"];
    // Get the temperature
    chat_history.set_temperature( std::stof( static_cast<const string&>(config_json["Temperature"]) ) );
    // Get the top k
    chat_history.set_top_k( std::stoi( static_cast<const string&>( config_json["TopK"] ) ) );
    // Get the top p
    chat_history.set_top_p( std::stof( static_cast<const string&>( config_json["TopP"] ) ) );


    // Dispose of the now-unused config_json
    raw_config_json_text.erase();
    config_json = NULL;

    // Append the api key to the message completion url
    message_completion_url += api_key;

    // Dispose of the now-unused api_key
    api_key.erase();

    // Create the window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Bison Chat");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create vertical box
    GtkWidget *message_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    // Set the spacing between items in the box
    gtk_box_set_spacing(GTK_BOX(message_vbox), 10);

    // Create scrolled window
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    // Have the message vbox fill the scrolled window
    gtk_container_add(GTK_CONTAINER(scrolled_window), message_vbox);

    // Create entry box and submit button
    GtkWidget *entry = gtk_entry_new();
    GtkWidget *submit_button = gtk_button_new_with_label("Submit");
    g_object_set_data(G_OBJECT(submit_button), "attached_entry_field", entry);
    g_object_set_data(G_OBJECT(entry), "attached_entry_field", entry);
    g_object_set_data(G_OBJECT(submit_button), "attached_message_vbox", message_vbox);
    g_object_set_data(G_OBJECT(entry), "attached_message_vbox", message_vbox);

    // Create horizontal box
    GtkWidget *entry_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(entry_hbox), entry, TRUE, TRUE, 5);
    gtk_box_pack_end(GTK_BOX(entry_hbox), submit_button, FALSE, FALSE, 0);

    // Create vertical box
    GtkWidget *window_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(window_vbox), scrolled_window, TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(window_vbox), entry_hbox, FALSE, FALSE, 0);
    
    //Create hbox for code execution prompt
    GtkWidget *window_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start(GTK_BOX(window_hbox), window_vbox, TRUE, TRUE, 0);

    // Add vbox to window
    gtk_container_add(GTK_CONTAINER(window), window_hbox);

    //instantiate UIElements struct
    UIElements *elements = new UIElements;
    elements->message_vbox = message_vbox;
    elements->scrolled_window = scrolled_window;
    elements->window_hbox = window_hbox;

    // Connect the submit button to the send_message callback
    g_signal_connect(submit_button, "clicked", G_CALLBACK(send_message), elements);

    // Connect the entry field to the send_message callback
    g_signal_connect(entry, "activate", G_CALLBACK(send_message), elements);

    // Show all widgets
    gtk_widget_show_all(window);

    // Add the function to the main loop to be called every 100 milliseconds
    g_timeout_add(250, main_loop_interface, NULL);

    // Run the main loop
    gtk_main();

    return 0;
}

/* ------------------------------------------------------------------------------------------
 * End of main function only section
 */