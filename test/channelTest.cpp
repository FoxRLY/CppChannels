#include <gtest/gtest.h>
#include <channel/channel.hpp>
#include <string>
#include <thread>

TEST(ChannelTests, TestSender_Receiver){
    channel::Channel<int> channel;
    std::string initial_message = "Hello there!";
    std::string received_message;

    auto sender = [&]{
        for (char message_char : initial_message) {
            channel.send(message_char);
        }
        channel.close();
    };

    auto receiver = [&]{
        for (char message_char : channel) {
            received_message.push_back(message_char);
        }
    };

    std::thread sender_thread(sender);
    std::thread receiver_thread(receiver);

    sender_thread.join();
    receiver_thread.join();

    ASSERT_EQ(initial_message, received_message);
}
