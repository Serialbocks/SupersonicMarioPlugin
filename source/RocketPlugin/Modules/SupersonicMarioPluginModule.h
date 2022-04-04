#pragma once
class SupersonicMarioPlugin;

class RocketPluginModule
{
    friend SupersonicMarioPlugin;
public:
    static SupersonicMarioPlugin* Outer()
    {
        return supersonicMarioPlugin;
    }

protected:
    static SupersonicMarioPlugin* supersonicMarioPlugin;
};
