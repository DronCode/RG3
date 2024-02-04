#pragma once


namespace samples::my_cool_sample
{
    /**
     * @runtime
     **/
    struct Type1Serializer
    {
    };

    /**
     * @runtime
     * @serializer(Type1Serializer)
     **/
    struct Type1
    {
        /**
         * @property(CoolField)
         */
        int field0;
    };
}