// Copyright 2018 Your Name <your_email>

#include <header.hpp>
boost::asio::io_service service;

struct talk_to_svr {
    explicit talk_to_svr(std::string  username)
            : sock_(service), username_(std::move(username))
    {}

    void connect(boost::asio::ip::tcp::endpoint ep) {
        sock_.connect(ep);
    }

    void loop() {
        write("login " + username_ + "\n");
        read_answer();
        while (true) {
            write_request();
            read_answer();
            int millis = rand() % 7000;
            boost::this_thread::sleep(boost::posix_time::millisec(millis));
            std::cout << username_ << " last ping "
                      << millis << " ms before" << std::endl;
        }
    }

    std::string username() const { return username_; }

private:
    void read_answer() {
        already_read_ = 0;
        read(sock_, boost::asio::buffer(buff_),
             boost::bind(&talk_to_svr::read_complete, this, _1, _2));
        process_msg();
    }

    void process_msg() {
        std::string msg(buff_, already_read_);

        if ( msg.find("login ") == 0) on_login();
        else if ( msg.find("ping") == 0) on_ping(msg);
        else if ( msg.find("clients ") == 0) on_clients(msg);
        else std::cerr << "invalid msg " << msg << std::endl;
    }

    void on_login() {
        std::cout << username_ << " logged in" << std::endl;
        do_ask_clients();
    }

    void on_ping(const std::string & msg) {
        std::istringstream in(msg);
        std::string answer;
        in >> answer >> answer;
        if ( answer == "client_list_changed")
            do_ask_clients();
    }

    void on_clients(const std::string & msg) {
        std::string clients = msg.substr(8);
        std::cout << username_ << ", new client list: " << clients;
    }

    void do_ask_clients() {
        write("ask_clients\n");
        read_answer();
    }

    void write_request() { write("ping\n"); }
    void write(const std::string & msg) {
        sock_.write_some(boost::asio::buffer(msg));
    }

    size_t read_complete(const boost::system::error_code & err, size_t bytes) {
        if ( err) return 0;
        already_read_ = bytes;
        bool found = std::find(buff_, buff_ + bytes, '\n') < buff_ + bytes;
        return found ? 0 : 1;
    }

    boost::asio::ip::tcp::socket sock_;
    int already_read_;
    char buff_[MAX_MSG];
    std::string username_;
};

void run_client(const std::string & client_name) {
    boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::address::from_string("127.0.0.1"), 8001);
    talk_to_svr client(client_name);
    try {
        client.connect(ep);
        client.loop();
    } catch(boost::system::system_error & err) {
        std::cout << "client terminated " << client.username()
                  << ": " << err.what() << std::endl;
    }
}

int main() {
    run_client("Aleksey");
}

