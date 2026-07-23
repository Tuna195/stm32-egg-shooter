#include <gui/model/Model.hpp>
#include <gui/model/ModelListener.hpp>
#include "cmsis_os.h" // Thư viện FreeRTOS CMSIS

// Lấy biến queue đã khai báo ở main.c
extern osMessageQueueId_t shootEventQueueHandle;

Model::Model() : modelListener(0)
{

}

void Model::tick()
{
    // Chỉ xử lý khi queue đã được khởi tạo
    if (shootEventQueueHandle != NULL)
    {
        uint8_t msg;
        // Kiểm tra xem có tin nhắn nào trong Queue không (Timeout = 0 để không block luồng GUI)
        if (osMessageQueueGet(shootEventQueueHandle, &msg, NULL, 0) == osOK)
        {
            // Có tín hiệu nút bắn từ ISR -> Gọi hàm bên Listener (Presenter)
            if (modelListener != nullptr)
            {
                // Giả định bạn đã có một hàm ví dụ như shootButtonPressed() ở ModelListener
                // Nếu chưa có, bạn có thể truyền thẳng trực tiếp hoặc tự thêm hàm này.
                modelListener->shootButtonPressed();
            }
        }
    }
}
