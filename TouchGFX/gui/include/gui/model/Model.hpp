#ifndef MODEL_HPP
#define MODEL_HPP

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();
    void setFinalScore(int s);
    int  getFinalScore() const { return finalScore; }
    int  getHighScore() const { return highScore; }
private:
    int finalScore = 0;
    int highScore = 0;
protected:
    ModelListener* modelListener;
};

#endif // MODEL_HPP
