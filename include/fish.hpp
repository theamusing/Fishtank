#pragma once

#include "gameObject.hpp"

template <size_t WIDTH, size_t HEIGHT>
class Fish : public GameObject<WIDTH, HEIGHT>
{
public:
    enum state
    {
        DASHING,
        FLOATING,
        TURNING,
        STRUGGLING
    };

    using GameObject<WIDTH, HEIGHT>::GameObject;

    void update(size_t frame) override
    {
        if (m_lastFrame == 0 || frame - m_lastFrame >= m_moveDuration)
        {
            m_lastFrame = frame;
            m_lastX = this->m_posX;
            m_lastY = this->m_posY;
            // switch movement
            switch (m_state)
            {
            case DASHING:
                // Update dashing state
                m_state = FLOATING;
                m_moveDuration = (size_t)(max(1.0f, getDistance() / (m_floatingVelocity / FPS)));
                break;
            case FLOATING:
                // Update floating state
                chooseTarget();
                if ((m_targetX - this->m_posX) * this->m_scaleX > 0)
                {
                    m_state = TURNING;
                    m_moveDuration = TURNING_FRAME_COUNT;
                }
                else
                {
                    m_state = DASHING;
                    m_moveDuration = DASHING_FRAME_COUNT;
                }
                break;
            case TURNING:
                // Update turning state
                this->m_scaleX = -this->m_scaleX;
                m_state = DASHING;
                m_moveDuration = DASHING_FRAME_COUNT;
                break;
            case STRUGGLING:
                // Update struggling state
                break;
            }
        }

        // Update state
        switch (m_state)
        {
        case DASHING:
            // Update dashing state
            this->m_currentFrame = (frame - m_lastFrame) % DASHING_FRAME_COUNT;
            this->m_posX += m_dashingVelocity * float(m_targetX - m_lastX) / getDistance() / FPS;
            this->m_posY += m_dashingVelocity * float(m_targetY - m_lastY) / getDistance() / FPS;
            break;
        case FLOATING:
            // Update floating state
            this->m_currentFrame = ((frame - m_lastFrame) % (2 * DASHING_FRAME_COUNT)) / 2;
            this->m_posX = m_lastX + (m_targetX - m_lastX) / float(m_moveDuration) * float(frame - m_lastFrame + 1);
            this->m_posY = m_lastY + (m_targetY - m_lastY) / float(m_moveDuration) * float(frame - m_lastFrame + 1);
            break;
        case TURNING:
            // Update turning state
            this->m_currentFrame = 0; // TODO: implement turning animation
            break;
        case STRUGGLING:
            // Update struggling state
            break;
        }
    }

    void draw(LGFX_Sprite &sprite, SpriteData &spriteData, ColorMap &colorMap) override
    {
        GameObject<WIDTH, HEIGHT>::draw(sprite, spriteData, colorMap);
        // display target point
        // sprite.drawCircle(m_targetX, m_targetY, 5, sprite.color565(255, 0, 0));
    }

protected:
    float m_dashingVelocity = 30.0f;
    float m_floatingVelocity = 10.0f;
    int m_targetX = 0;
    int m_targetY = 0;
    int m_lastX = 0;
    int m_lastY = 0;
    state m_state = FLOATING;
    size_t m_lastFrame = 0;
    size_t m_moveDuration = 5;
    size_t DASHING_FRAME_COUNT = 4;
    size_t TURNING_FRAME_COUNT = 1;
    size_t STRUGGLING_FRAME_COUNT = 2;

    void chooseTarget()
    {
        bool turn = !(this->m_posX < 10 && this->m_scaleX < 0 || this->m_posX > RENDER_WIDTH - 10 && this->m_scaleX > 0) && (min(this->m_posX, RENDER_WIDTH - this->m_posX) < random(10, 80));
        float deltaX = random(20, 60);
        m_targetX = turn ? this->m_posX + deltaX * this->m_scaleX : this->m_posX - deltaX * this->m_scaleX;

        // m_targetX = max(10, min(RENDER_WIDTH - 10, m_targetX));

        if (m_targetY < 20)
        {
            m_targetY = this->m_posY + random(0, deltaX / 5);
        }
        else if (m_targetY > RENDER_HEIGHT - 30)
        {
            m_targetY = this->m_posY - random(0, deltaX / 5);
        }
        else
        {
            m_targetY = this->m_posY + random(-deltaX / 5, deltaX / 5);
        }
    }

    float getDistance()
    {
        float dist = sqrt((m_targetX - m_lastX) * (m_targetX - m_lastX) + (m_targetY - m_lastY) * (m_targetY - m_lastY));
        if ((m_targetX - m_lastX) * this->m_scaleX > 0)
        {
            dist *= -1;
        }
        return dist;
    }
};

// GameObject<20, 12> clownfish;
// GameObject<16, 10> guppy;
// GameObject<19, 6> longfish;

class ClownFish : public Fish<20, 12>
{
public:
    ClownFish() : Fish<20, 12>()
    {
        DASHING_FRAME_COUNT = 5;
        m_dashingVelocity = 25.0f;
        m_floatingVelocity = 15.0f;
    };
};

class Guppy : public Fish<16, 10>
{
public:
    Guppy() : Fish<16, 10>()
    {
        DASHING_FRAME_COUNT = 4;
        m_dashingVelocity = 30.0f;
        m_floatingVelocity = 20.0f;
    };
};

class LongFish : public Fish<19, 6>
{
public:
    LongFish() : Fish<19, 6>()
    {
        DASHING_FRAME_COUNT = 4;
        m_dashingVelocity = 15.0f;
        m_floatingVelocity = 10.0f;
    };
};