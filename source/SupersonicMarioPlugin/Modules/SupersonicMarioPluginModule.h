#pragma once
class SupersonicMarioPlugin;

class SupersonicMarioPluginModule
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
